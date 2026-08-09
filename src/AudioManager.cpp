#include "AudioManager.h"

#include <iostream>

AudioManager::AudioManager()
{
}

AudioManager::~AudioManager()
{
    destroySounds();

    if (available)
    {
        Mix_CloseAudio();
        Mix_Quit();
    }
}

bool AudioManager::initialize()
{
    if (
        Mix_OpenAudio(
            44100,
            MIX_DEFAULT_FORMAT,
            2,
            2048) < 0)
    {
        std::cerr
            << "SDL_mixer error: "
            << Mix_GetError()
            << std::endl;

        available = false;

        return false;
    }

    available = true;

    loadSounds();

    return true;
}

Mix_Chunk* AudioManager::loadSound(
    const std::string& path)
{
    Mix_Chunk* sound =
        Mix_LoadWAV(
            path.c_str());

    if (!sound)
    {
        std::cerr
            << "Could not load sound: "
            << path
            << "\n"
            << Mix_GetError()
            << std::endl;
    }

    return sound;
}

void AudioManager::loadSounds()
{
    if (!available)
    {
        return;
    }

    const std::string base =
        "assets/sounds/standard/";

    sounds["move-white"] =
        loadSound(
            base +
            "move-self.mp3");

    sounds["move-black"] =
        loadSound(
            base +
            "move-opponent.mp3");

    sounds["capture"] =
        loadSound(
            base +
            "capture.mp3");

    sounds["castle"] =
        loadSound(
            base +
            "castle.mp3");

    sounds["check"] =
        loadSound(
            base +
            "move-check.mp3");

    sounds["illegal"] =
        loadSound(
            base +
            "illegal.mp3");

    sounds["promotion"] =
        loadSound(
            base +
            "promote.mp3");

    sounds["start"] =
        loadSound(
            base +
            "game-start.mp3");

    sounds["checkmate"] =
        loadSound(
            base +
            "game-win-long.mp3");

    sounds["draw"] =
        loadSound(
            base +
            "game-draw.mp3");

    sounds["click"] =
        loadSound(
            base +
            "click.mp3");
}

void AudioManager::play(
    const std::string& name)
{
    if (!available)
    {
        return;
    }

    auto it =
        sounds.find(name);

    if (
        it ==
        sounds.end())
    {
        return;
    }

    if (!it->second)
    {
        return;
    }

    Mix_PlayChannel(
        -1,
        it->second,
        0);
}

void AudioManager::playMoveSound(
    MoveSound sound)
{
    switch (sound)
    {
        case MoveSound::MoveWhite:
            play("move-white");
            break;

        case MoveSound::MoveBlack:
            play("move-black");
            break;

        case MoveSound::Capture:
            play("capture");
            break;

        case MoveSound::Castle:
            play("castle");
            break;

        case MoveSound::Check:
            play("check");
            break;

        case MoveSound::Illegal:
            play("illegal");
            break;

        case MoveSound::Promotion:
            play("promotion");
            break;

        case MoveSound::Checkmate:
            play("checkmate");
            break;

        case MoveSound::Draw:
            play("draw");
            break;

        case MoveSound::None:
            break;
    }
}

bool AudioManager::isAvailable() const
{
    return available;
}

void AudioManager::destroySounds()
{
    for (auto& item : sounds)
    {
        if (item.second)
        {
            Mix_FreeChunk(
                item.second);
        }
    }

    sounds.clear();
}