#include <iostream>
#include "include/gme.h"
#include <string>
#include <fstream>
#include "include/tinydir.h"

#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

#define SAMPLE_RATE 44100

Music_Emu* emu;

const char* lastError;
const char* lastTrackError;

bool loaded = false;

void loadFile(const char * path)
{
    // Determine file type.
    gme_type_t file_type;
    lastError = gme_identify_file(path, &file_type );

    if( lastError )
    {
        return;
    }

    if ( !file_type )
    {
        lastError = "Unsupported music type";
        return;
    }

    const char * console_name = gme_type_system(file_type);

    cout << "Console: " << console_name << endl;

    // Create emulator and set sample rate.
    gme_open_file(path, &emu, SAMPLE_RATE);
    if ( !emu )
    {
        lastError = "Out of memory";
        return;
    }

    // Get track info for the first track.
    gme_info_t* track_info;
    gme_track_info(emu, &track_info, 0);

    const char * game_title = track_info->game;
    cout << "Game: " << game_title << endl;

    // Release the info struct.
    gme_free_info(track_info);

    // Disable silence detection.
    gme_ignore_silence(emu, 1);

    loaded = true;
}

void setFadeTime(int track_number)
{
    // Get track info so we can find track length.
    gme_info_t* track_info;
    gme_track_info(emu, &track_info, track_number);

    int track_length = track_info->length;
    int intro_length = track_info->intro_length;
    int loop_length = track_info->loop_length;

    // Release the info struct.
    gme_free_info(track_info);

    int fade_time_ms = 0;

    // Use track length if specified.
    if ( track_length > 0 )
    {
        fade_time_ms = track_length;
    }

    // If a loop length is specified, loop twice after the intro section.
    if ( loop_length > 0 )
    {
        fade_time_ms = intro_length + (2 * loop_length);
    }

    // If neither of the above specified, play for 2.5 minutes.
    if ( fade_time_ms == 0 )
    {
        fade_time_ms = 150000;
    }

    // Set fade out on this track.
    gme_set_fade(emu, fade_time_ms);
}

int startTrack(int track_number)
{
    // Start track.
    lastError = gme_start_track(emu, track_number);

    if( lastError )
    {
        return -1;
    }

    setFadeTime(track_number);
    return 0;
}

int getTrackCount()
{
    int result = gme_track_count(emu);
    return result;
}

int getVoiceCount()
{
    int result = gme_voice_count(emu);

    if (result == 0)
    {
        return -1;
    }

    return result;
}

string getVoiceName(int voice_number)
{
    const char* voice_name = gme_voice_name(emu, voice_number);
    return string(voice_name);
}


string getFileName(const string& strPath)
{
    size_t index_last_separator = strPath.find_last_of("/\\") + 1;
    size_t index_last_period = strPath.find_last_of(".");

    if ((index_last_separator > 0) && (index_last_period > index_last_separator))
    {
        return strPath.substr(index_last_separator, index_last_period - index_last_separator);
    }
    else
    {
        return NULL;
    }
}

string getExtension(const string& strPath)
{
    size_t index_last_separator = strPath.find_last_of("/\\") + 1;
    size_t index_last_period = strPath.find_last_of(".");

    if ((index_last_separator > 0) && (index_last_period > index_last_separator))
    {
        return strPath.substr(index_last_period);
    }
    else
    {
        return NULL;
    }
}

void writeLittleEndian(unsigned int word, int num_bytes, FILE *wav_file)
{
    unsigned buf;
    while(num_bytes > 0)
    {
        buf = word & 0xff;
        fwrite(&buf, 1, 1, wav_file);
        num_bytes--;
        word >>= 8;
    }
}

void writeTrack(int track_index, int voice_index, int voice_count, string input_filename)
{
    // Mute all the other tracks.
    for (int mute_voice_index = 0; mute_voice_index < voice_count; mute_voice_index++)
    {
        // True indicates that we do want to mute this track.
        gme_mute_voice(emu, mute_voice_index, true);
    }

    // Unmute the one track we want this time.
    gme_mute_voice(emu, voice_index, false);

    startTrack(track_index);

    string separator = string(" - ");
    string title;

    // Get track info.
    gme_info_t* track_info;
    gme_track_info(emu, &track_info, 0);

    string song_title = track_info->song;
    if (song_title.empty())
    {
        // Use the file name with the track number appended.
        char track_number_string [4];
        sprintf(track_number_string, "%d", track_index);
        song_title = input_filename.append(" Track ");
        song_title = input_filename.append(track_number_string);
    }

    // Release the info struct.
    gme_free_info(track_info);

    string output_filename = "out/" + song_title + separator + getVoiceName(voice_index) + string(".wav");

    cout << "Writing voice #" << voice_index + 1 << " to file " << output_filename << "." << endl;

    // Open the output file.
    FILE * file_pointer = fopen(output_filename.c_str(), "w");

    unsigned int num_channels = 2;
    unsigned int bytes_per_sample = 2;
    unsigned int byte_rate = SAMPLE_RATE * num_channels * bytes_per_sample;

    // Write RIFF header.
    fwrite("RIFF", 1, 4, file_pointer);
    long int total_size_offset = ftell(file_pointer);
    writeLittleEndian(36 + bytes_per_sample * 1 * num_channels, 4, file_pointer);
    fwrite("WAVE", 1, 4, file_pointer);

    // Write fmt subchunk.
    fwrite("fmt ", 1, 4, file_pointer);
    writeLittleEndian(16, 4, file_pointer);   /* SubChunk1Size is 16 */
    writeLittleEndian(1, 2, file_pointer);    /* PCM is format 1 */
    writeLittleEndian(num_channels, 2, file_pointer);
    writeLittleEndian(SAMPLE_RATE, 4, file_pointer);
    writeLittleEndian(byte_rate, 4, file_pointer);
    writeLittleEndian(num_channels * bytes_per_sample, 2, file_pointer);  /* block align */
    writeLittleEndian(8 * bytes_per_sample, 2, file_pointer);  /* bits/sample */

    // Write data subchunk.
    fwrite("data", 1, 4, file_pointer);
    long int data_size_offset = ftell(file_pointer);
    writeLittleEndian(bytes_per_sample * 1 * num_channels, 4, file_pointer);

    size_t bytes_written = 0;

    // Loop until track ends
    while (!gme_track_ended(emu))
    {
        short int buffer [SAMPLE_RATE];
        gme_play(emu, SAMPLE_RATE, buffer);

        int elements_written = fwrite(buffer, sizeof(short), SAMPLE_RATE, file_pointer);

        bytes_written += elements_written * sizeof(short);
    }

    // Write the data size to the header.
    fseek(file_pointer, total_size_offset, SEEK_SET);
    writeLittleEndian(36 + bytes_written, 4, file_pointer);

    fseek(file_pointer, data_size_offset, SEEK_SET);
    writeLittleEndian(bytes_written, 4, file_pointer);

    // Close the file.
    fclose(file_pointer);

    gme_seek(emu, 0);
}

int writeFile(tinydir_file file)
{
    cout << "Opening file: " << file.name << endl;

    string input_filename = file.path;

    if (input_filename.empty())
    {
        cout << "ERROR: Bad file name." << endl;
        return -1;
    }

    loadFile(file.path);

    if (!loaded)
    {
        cout << "ERROR: Could not load file." << endl;
        return -1;
    }

    int track_count = getTrackCount();
    cout << "File contains " << track_count << " tracks." << endl;

    // For each track in the file,
    for (int track_index = 0; track_index < track_count; track_index++)
    {
        // Get track info.
        gme_info_t* track_info;
        gme_track_info(emu, &track_info, track_index);

        const char * artist = track_info->author;
        const char * song_title = track_info->song;

        if (string(song_title).empty())
        {
            cout << "Track " << track_index + 1 << endl;
        }
        else
        {
            cout << "Track " << track_index + 1 << ": " << artist << " - " << song_title << endl;
        }

        // Release the info struct.
        gme_free_info(track_info);

        int voice_count = getVoiceCount();
        cout << "Track contains " << voice_count << " voices." << endl;

        // For each voice in the track,
        for (int voice_index = 0; voice_index < voice_count; voice_index++)
        {
            writeTrack(track_index, voice_index, voice_count, input_filename);
        }

        cout << "All voices written successfully!" << endl;
    }

    cout << "All tracks written successfully!" << endl;

    delete emu;
}

int main()
{
    cout << "VGMSplitter v0.1" << endl;

    // Make sure input directory exists.
    struct stat st = {0};
    if (stat("in", &st) == -1) {
        mkdir("in", 0700);
    }

    if (stat("out", &st) == -1) {
        mkdir("out", 0700);
    }

    // Get list of files in input directory.
    tinydir_dir dir;
    tinydir_open(&dir, "./in");

    // While there are more files to read in this directory,
    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);

        if (!file.is_dir)
        {
            writeFile(file);
        }

        tinydir_next(&dir);
    }

    cout << "All files split successfully!" << endl;

    tinydir_close(&dir);

    return 0;
}
