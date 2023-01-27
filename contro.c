// COMPILE: $ gcc contro.c -o contro -lSDL2 -lSDL2_image `sdl2-config --cflags --libs` --std=c99 -Wall -g
// FIX png files with ImageMagick: $ mogrify ./*.png

#include "SDL.h"
#include "SDL_image.h"
#include <stdio.h>

#define MAX_BULLETS 3 // max bullets per time

typedef struct // character attributes
{
	float x, y, dx, dy; // speed left, right, jump (vertical speed)
	short life; // lifes
	char *name;
	int currentSprite, walking, facingLeft, shooting, visible, landed, lying_down;
	int alive; // alive

	SDL_Texture *sheetTexture; // the spritesheet
} Man;

typedef struct // bullet attributes
{
	float x, y, dx;
} Bullet;

typedef struct
{
	unsigned short left_pressed;
	unsigned short right_pressed;
	float inertia;
} Controls;

SDL_Texture *bulletTexture; // bullet sprite
SDL_Texture *backgroundTexture; // level background
Bullet *bullets[MAX_BULLETS] = { NULL };
Man enemy;

int globalTime = 0;

void addBullet(float x, float y, float dx)
{
	int found = -1;
	for(int i = 0; i < MAX_BULLETS; i++)
	{
		if(bullets[i] == NULL)
		{
			found = i;
			break; // stop executing
		}
	}

	if(found >= 0)
	{
		int i = found;
		bullets[i] = malloc(sizeof(Bullet));
		bullets[i]->x = x;
		bullets[i]->y = y;
		bullets[i]->dx = dx;
	}
}

void removeBullet(int i)
{
	if(bullets[i])
	{
		free(bullets[i]);
		bullets[i] = NULL;
	}
}

// processing
int processEvents(SDL_Window *window, Man *man, Controls *control)
{
	SDL_Event event;  // event data
	int done = 0;

	// escape and quit events
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_WINDOWEVENT_CLOSE:
			{
				if(window)
				{
					SDL_DestroyWindow(window);
					window = NULL;
					done = 1;
				}
			}
			break;

			case SDL_KEYDOWN:
			{
				switch(event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
					{
						done = 1;
					}
					break;
				}
			}
			break;

			case SDL_QUIT:
			{
				//quit out of the game
				done = 1;
				//break;
			}
			break;
		}
    }

	//printf("DEB-7 shooting = %d\n", man->shooting);
	// keyboard input
	const Uint8 *state = SDL_GetKeyboardState(NULL);
	if(!man->shooting) // if player is not shooting (player can to walk)
	{
		if(state[SDL_SCANCODE_LEFT] && !man->lying_down) // run to the left while pressing keyboard button
		{
			control->right_pressed = 0;
			//printf("DEB-7 lying_down = %d\n", man->lying_down);
			man->x = man->x - 0.75; // add speed x (walking speed)
			man->walking = 1; // walking true
			man->facingLeft = 1; // facing to the left true

			// sprite transition/animation speed
			if(globalTime % 11 == 0) // every 6 seconds (animation speed)
			{
				man->currentSprite++;

				// in that update time, we only will draw 5 first images from spritesheet
				man->currentSprite %= 5;
				//printf("DEB-1 man->currentSprite = %d\n", man->currentSprite);
			}
			control->left_pressed = 1;
		}
		else if(state[SDL_SCANCODE_RIGHT] && !man->lying_down) // same as above, but facing right
		{
			control->left_pressed = 0;

			man->x = man->x + 0.75;
			man->walking = 1;
			man->facingLeft = 0;

			if(globalTime % 11 == 0)
			{
				man->currentSprite++;
				man->currentSprite %= 5; // the currentSprite is setting to
				//man -> currentSprite = man -> currentSprite % 4;
				//printf("DEB-2 man->currentSprite = %d\n", man->currentSprite);
			}
			control->right_pressed = 1;
		}
		else // if no left or right buttons are pressed (idle)
		{
			/*
			if(globalTime % 2 == 0)
			{
				if ((control->left_pressed == 1) && (man->x < 0.0))
				{
					man->x = man->x + 0.01;
					//control->left_pressed = 0;
				}
				if ((control->right_pressed == 1) && (man->x > 0.0))
				{
					man->x = man->x - 0.01;
				}
			}*/

			//if(globalTime % 4 == 0)
			//{
				control->left_pressed = 0;
				control->right_pressed = 0;
			//}

			man->walking = 0; // walking false
			man->currentSprite = 6; // set sprite number 4 from spritesheet (0-5)(idle)
			//printf("DEB-3 man->currentSprite = %d\n", man->currentSprite);
		}
	}

	if(!man->walking) // if player is not walking (player can shoot)
	{
		if(state[SDL_SCANCODE_SPACE])// && !man->dy) // if pressing SPACE
		{
			if(globalTime % 6 == 0)
			{
				if(man->currentSprite == 5) // if sprite is idle
				{
					man->currentSprite = 6; // shooting and recoiling prite
					//printf("DEB-4 man->currentSprite = %d\n", man->currentSprite);
				}
			/* if already shooting and recoiling, set back to idle, simulating
			the recoil */
				else
				{
					man->currentSprite = 5; // idle sprite
					//printf("DEB-5 man->currentSprite = %d\n", man->currentSprite);
				}

				if(!man->facingLeft) // if player not facing to the left
				{ // shoot a bullet from the gun position, and speed bullet/direction
					addBullet(man->x+20, man->y+12, 2);
				}
				else // if man facing to the left
				{
					addBullet(man->x-3, man->y+12, -2);
				}
			}
			man->shooting = 1; // player shooting true
		}
		else // not pressing SPACE
		{
			man->currentSprite = 5; // set sprite back to idle state
			//printf("DEB-6 man->currentSprite = %d\n", man->currentSprite);
			man->shooting = 0; // shooting is false
		}
	}


	// player jump, dy is vertical speed
	// if player is pressing SPACE, landed and not lying down
	if(state[SDL_SCANCODE_UP] && man->landed==1 && !man->lying_down)// man->dy>=0) // !man->dy)
	{
		if(globalTime % 12 == 0)
		{
			man->dy = -5; // jump speed
			man->landed = 0; // man is jumping and not landed
			man->lying_down = 0;
		}
	}

	if(state[SDL_SCANCODE_DOWN] && man->landed==1)
	{
		//man->y += 10; // get down
		man->landed = 1;
		man->lying_down = 1;
	}
	else if(!state[SDL_SCANCODE_DOWN] && man->landed==1)
	{
		man->landed = 1;
		man->lying_down = 0;
	}

	if(state[SDL_SCANCODE_DOWN] && state[SDL_SCANCODE_UP] &&
		man->landed==1 && man->lying_down)
	{
		man->y = man->y + 10; // get down
		man->landed = 0;
		man->lying_down = 0;
	}
	//printf("DEB-7 lying_down = %d\n", man->lying_down);

	return done;
}

void doRender(SDL_Renderer *renderer, Man *man)
{
  //set the drawing color to blue
  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

  //Clear the screen (to blue)
  SDL_RenderClear(renderer);

  //set the drawing color to white
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  //SDL_RenderFillRect(renderer, &rect);
  SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);

  //warrior
  if(man->visible) // if player is visible, we render it
  { // Max frame width * frame number, starts from 0 y point, frame width, frame heigh
    SDL_Rect srcRect = { 25*man->currentSprite, 0, 24, 37 }; // fit the sprite
    SDL_Rect rect = { man->x, man->y, 24, 37 }; // sprite/player frame size

    // spawn player with this parameters
    SDL_RenderCopyEx(renderer, man->sheetTexture, &srcRect, &rect, 0, NULL, man->facingLeft);
  }

  //enemy
  if(enemy.visible)
  {
    SDL_Rect eSrcRect = { 40*enemy.currentSprite, 0, 40, 50 };
    SDL_Rect eRect = { enemy.x, enemy.y, 40, 50 };
    SDL_RenderCopyEx(renderer, enemy.sheetTexture, &eSrcRect, &eRect, 0, NULL, enemy.facingLeft);
  }

  // render the bullets
  for(int i = 0; i < MAX_BULLETS; i++) if(bullets[i])
  {
    SDL_Rect rect = { bullets[i]->x, bullets[i]->y, 8, 8 };
    SDL_RenderCopy(renderer, bulletTexture, NULL, &rect);
  }

  //We are done drawing, "present" or show to the screen what we've drawn
  SDL_RenderPresent(renderer);
}

void updateLogic(Man *man, Controls *control)
{
	//if(globalTime % 2 == 0) // gravity effect (some lag)
	//{
		if(man->facingLeft && control->left_pressed)
		{
			man->x = man->x + man->dy - 0.5;
			//man->x = man->x - man->dx; // move left
			//man->dx = man->dx + control->inertia; // increase speed
		}
		else if(!man->facingLeft && control->right_pressed)
		{
			man->x = man->x + man->dy + 0.5;
			//man->x = man->x + man->dx; // move right
			//man->dx = man->dx + control->inertia; // increase speed
		}
		else
		{
			man->dx = man->dx * 0.9; // decrease speed if no arrow key is pressed
		}
		//man->x = man->x + man->dy * 0.2; // walk intertia
		man->y = man->y + man->dy + 2; // gravity
		man->dy = man->dy + 0.09; // jump height
	//}

  // floor collision
  if(man->y > 70 && !man->lying_down)
  {
    man->y = 70;
    man->dy = 0;
    man->landed = 1;
  }

  for(int i = 0; i < MAX_BULLETS; i++) if(bullets[i])
  {
    bullets[i]->x += bullets[i]->dx;

    //simple coll. detection
    if(bullets[i]->x > enemy.x && bullets[i]->x < enemy.x+40 &&
       bullets[i]->y > enemy.y && bullets[i]->y < enemy.y+50)
    {
      enemy.alive = 0;
    }

    if(bullets[i]->x < -1000 || bullets[i]->x > 1000)
      removeBullet(i);
  }

  // enemy death, simulate explosion
  if(enemy.alive == 0 && globalTime % 6 == 0)
  {
    if(enemy.currentSprite < 6)
      enemy.currentSprite = 6;
    else if(enemy.currentSprite >= 6)
    {
      enemy.currentSprite++;
      if(enemy.currentSprite > 7)
      {
        enemy.visible = 0;
        enemy.currentSprite = 7;
      }
    }
  }

  // respawn enemy after death
  if(enemy.alive == 0 && globalTime % 256 == 0)
  {
	enemy.x = 250;
	enemy.y = 60;
	enemy.currentSprite = 4;
	enemy.facingLeft = 1;
	enemy.alive = 1;
	enemy.visible = 1;
	enemy.landed = 1;
	enemy.lying_down = 0;
  }

  globalTime++;
}

int main(int argc, char *argv[])
{
  SDL_Window *window;                    // Declare a window
  SDL_Renderer *renderer;                // Declare a renderer

  SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2

  Controls control;
  control.inertia = 0.9;

  // spawn player
  Man man;
  man.x = 50;
  man.y = 0;
  man.currentSprite = 4;
  man.alive = 1;
  man.visible = 1;
  man.facingLeft = 0;
  man.landed = 1;
  man.lying_down = 0;
  man.shooting = 0;

  // spawn enemy
  enemy.x = 250;
  enemy.y = 60;
  enemy.currentSprite = 4;
  enemy.facingLeft = 1;
  enemy.alive = 1;
  enemy.visible = 1;
  enemy.landed = 1;
  enemy.lying_down = 0;
  enemy.shooting = 0;

  //Create an application window with the following settings:
  window = SDL_CreateWindow("Contro",                     // window title
                            SDL_WINDOWPOS_UNDEFINED,           // initial x position
                            SDL_WINDOWPOS_UNDEFINED,           // initial y position
                            640,                               // width, in pixels
                            480,                               // height, in pixels
                            0                                  // flags
                            );
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  SDL_RenderSetLogicalSize(renderer, 320, 240);


  // load player spritesheet
  SDL_Surface *sheet = IMG_Load("sheet.png");
  if(!sheet)
  {
    printf("Cannot find sheet\n");
    return 1;
  }

  man.sheetTexture = SDL_CreateTextureFromSurface(renderer, sheet);
  SDL_FreeSurface(sheet);

  //load enemy
  sheet = IMG_Load("badman_sheet.png");
  if(!sheet)
  {
    printf("Cannot find badman_sheet\n");
    return 1;
  }

  enemy.sheetTexture = SDL_CreateTextureFromSurface(renderer, sheet);
  SDL_FreeSurface(sheet);

  //load the bg
  SDL_Surface *bg = IMG_Load("background.png");

  if(!bg) //if(!sheet)
  {
    printf("Cannot find background\n");
    return 1;
  }

  backgroundTexture = SDL_CreateTextureFromSurface(renderer, bg);
  SDL_FreeSurface(bg);

  //load the bullet
  SDL_Surface *bullet = IMG_Load("bullet.png");

  if(!bullet)
  {
    printf("Cannot find bullet\n");
    return 1;
  }

  bulletTexture = SDL_CreateTextureFromSurface(renderer, bullet);
  SDL_FreeSurface(bullet);

  // The window is open: enter program loop (see SDL_PollEvent)
  int done = 0;

  //Event loop
  while(!done) // will keep running if 0
  {
    //Check for events
    done = processEvents(window, &man, &control);

    //Update logic
    updateLogic(&man, &control);

    //Render display
    doRender(renderer, &man);

    //don't burn up the CPU
    SDL_Delay(10);
  }


  // Close and destroy the window
  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyTexture(man.sheetTexture);
  SDL_DestroyTexture(backgroundTexture);
  SDL_DestroyTexture(bulletTexture);
  SDL_DestroyTexture(enemy.sheetTexture);

  for(int i = 0; i < MAX_BULLETS; i++)
    removeBullet(i);

  // Clean up
  SDL_Quit();
  return 0;
}

