
#include<cmath>
#include<cstdlib>
#include<iostream>
#include<SDL2/SDL.h>

#include"sdl.hpp"

using namespace std;

namespace SDL
{

	/** The screen resolution **/
	unsigned int window_w;
	unsigned int window_h;

	/** The image resolution **/
	int image_w;
	int image_h;

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;

	bool die = false;

	int initSDL(unsigned int w, unsigned int h)
	{
		cerr << "[SDL] Loading SDL" << endl;

	  image_w = w;
	  image_h = h;

		if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
		{
			cerr << "[SDL] Video initialization failed: " << SDL_GetError() << endl;
			return 1;
		}

	  /*const SDL_VideoInfo* info = SDL_GetVideoInfo();
	  window_w = info->current_w - 100;
	  window_h = info->current_h - 100;

		int w_ratio = window_w / image_w;
		int h_ratio = window_h / image_h;

	  int ppp = w_ratio < h_ratio ? w_ratio : h_ratio;
	  window_w = ppp * image_w;
	  window_h = ppp * image_h;*/

		window = SDL_CreateWindow("RayTracer", 100, 100, image_w, image_h, SDL_WINDOW_SHOWN);
		if(window == nullptr)
		{
			cerr << "[SDL] Could not create window: " << SDL_GetError() << endl;
			SDL_Quit();
			return 1;
		}

		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
		if(renderer == nullptr)
		{
			cerr << "[SDL] Could not create renderer: " << SDL_GetError() << endl;
			SDL_DestroyWindow(window);
			SDL_Quit();
			return 1;
		}

		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_STREAMING, image_w, image_h);
		if(texture == nullptr)
		{
			cerr << "[SDL] Could not create texture: " << SDL_GetError() << endl;
			SDL_DestroyWindow(window);
			SDL_DestroyRenderer(renderer);
			SDL_Quit();
			return 1;
		}

		cerr << "[SDL] Done loading SDL" << endl;
		return 0;
	}

	void closeSDL(void)
	{
		cerr << "[SDL] Closing SDL" << endl;
		SDL_DestroyTexture(texture);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		cerr << "[SDL] SDL closed" << endl;
	}

	void handleEvents(void)
	{
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_QUIT:
				cerr << "[GRAPHICS] Close requested" << endl;
				die = 1;
				break;
			case SDL_KEYDOWN:
				if(event.key.keysym.sym == SDLK_ESCAPE)
					die = true;
				return;
			default:
				break;
			}
		}
	}

	void wait(uint32_t ms)
	{
	  SDL_Delay(ms);
	}

	void drawFrame(uint32_t* pixels)
	{
		SDL_RenderClear(renderer);
		SDL_UpdateTexture(texture, 0, pixels, image_w * sizeof(uint32_t));
		SDL_RenderCopy(renderer, texture, 0, 0);
		SDL_RenderPresent(renderer);
	}
}
