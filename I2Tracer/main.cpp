#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "Parser.h"
#include "Core.h"

int main (int argc, char** argv) try {
    i2t::SceneData scene;
    //i2t::parse (scene, "scene4-ambient.test");    
    //i2t::parse (scene, "scene4-emission.test");
    //i2t::parse (scene, "scene4-diffuse.test");
    //i2t::parse (scene, "scene4-specular.test");
    //i2t::parse (scene, "scene5.test");
    //i2t::parse (scene, "scene6.test");
    i2t::parse (scene, "foo.test");
    //i2t::parse (scene, "scene7.test");

    //i2t::parse (scene, "foo.test");
    i2t::Core core (scene);

    SDL_Init (SDL_INIT_EVERYTHING);
    std::atexit (SDL_Quit);

    auto pWindow = SDL_CreateWindow (argv [0],
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        scene.camera ().size.x, 
        scene.camera ().size.y,
        SDL_WINDOW_SHOWN);

    auto pSurface = SDL_GetWindowSurface (pWindow);
    
    std::thread _render_thread ([&core] () {
        core.render ();
    });

    for (;;) {
        SDL_Event ev;
        if (SDL_PollEvent (&ev)) {
            if (ev.type == SDL_QUIT)
                break;
            if (ev.type == SDL_MOUSEBUTTONDOWN) {
                std::cout << ev.button.x << ", ";
                std::cout << ev.button.y << "\n";
            }
            continue;
        }
        SDL_Delay (16);
        SDL_LockSurface (pSurface);

        core.snapshot (
            i2t::Core::RGBA32,
            pSurface->pixels, 
            pSurface->w, 
            pSurface->h);

        SDL_UnlockSurface (pSurface);
        SDL_UpdateWindowSurface (pWindow);
    }
    SDL_SaveBMP (pSurface, (scene.output () + ".bmp").c_str ());
    SDL_DestroyWindow (pWindow);
    _render_thread.detach ();
    return 0;
}
catch (std::exception& e) {
    std::cout << e.what () << "\n";
    return -1;
}