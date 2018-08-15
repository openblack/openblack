#include <Game.h>
#include <SDL.h>
#include <Windows.h>

int main(int argc, char **argv)
{
    try
    {
        std::unique_ptr<OpenBlack::Game> game(new OpenBlack::Game(argc, argv));
        game->Run();
    }
    catch (std::runtime_error &e) {
        // Only catch runtime_error as these should be user issues.
        // Catching other types would just make debugging them more difficult.

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal error", e.what(), nullptr);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#if defined(_WIN32) && !defined(_CONSOLE)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    return main(__argc, __argv);
}
#endif
