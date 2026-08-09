#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "ChessTypes.h"

#include <SDL_mixer.h>

#include <map>
#include <string>

class AudioManager
{
public:
    AudioManager();

    ~AudioManager();

    bool initialize();

    void loadSounds();

    void play(
        const std::string& name);

    void playMoveSound(
        MoveSound sound);

    bool isAvailable() const;

private:
    bool available = false;

    std::map<
        std::string,
        Mix_Chunk*>
        sounds;

    Mix_Chunk* loadSound(
        const std::string& path);

    void destroySounds();
};

#endif