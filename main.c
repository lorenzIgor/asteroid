/* *************************** Ast3r0id ***************************   */
/* Author: V3lorek / ilorenz                  				       		      */
/* Date : 2020				               		                              */
/* [+] STRUCTURE: 		|-------------------v		                        */
/* [MAIN] -> [NEWGAME] -> [MAIN_LOOP] ->[UPDATEGAME && HANDLE_EVENTS] */
/*				^	            |	              */
/*                     [DRAWTOSCREEN] <-      [MOVEOBJECTS]           */
/* *****************************************************************  */
/* [Authors of Assets]		      				                              */
/* - Ship: [Seki]from cleanpng.com    				                        */
/* - Asteroid:  pngwave.com	      				                            */
/* - Theme: [Jan125] opengameart.org                                  */
/* - Crash : [freesound]                                              */
/* - Intro : https://mixkit.co/free-sound-effects/intro/    	        */
/* *****************************************************************  */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <time.h>
#include <sys/time.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_vulkan.h>
#include "list.h"
#include "ship.h"
#include "display.h"

#define PI 3.14159265
// GAME CONSTANTS
// #define screen.width 1920
// #define screen.height 1080
#define ASTW0 80
#define ASTH0 80
#define ASTW1 60
#define ASTH1 60
#define ASTW2 32
#define ASTH2 32
#define MAX_LIFE 100
#define MAX_KEY 1000
#define UP 1
#define DOWN 0
#define SPEED 800
#define ROTATION 350
#define PROJECTILE_SPEED 1000
#define PROJECTILE_RATE 100
#define ASTEROID_SPEED 50
#define ASTEROID_ROTATION 35
#define FPS_RATE 2400

// MESSAGES
#define STS_MSG0 "SDL Graphics loaded successfully.\n"
#define ERR_MSG0 "SDL Graphics could not be loaded. \n"
#define ERR_MSG1 "Failed to load asset. \n"

// #define GLFW_INCLUDE_VULKAN
// #include <GLFW/glfw3.h>
// #include <cglm/cglm.h>
// #include <cglm/mat4.h>

typedef struct _sprite
{
  SDL_Surface *Img;
} SPRITE;

/* GLOBALS */
OBJECT ship;
OBJECT *asteroids;
OBJECT *projectiles;
SPRITE shipSprite[9];
SDL_Surface *background, *asteroid, *projectile, *explosionIMG, *debris, *indicators;
SDL_Event ev;
SDL_Renderer *ren1;
SDL_Window *win1;
long KeyState[MAX_KEY];
BOOL Running = TRUE;
BOOL keypressed = FALSE;
BOOL mouseclicked = FALSE;
time_t t;
Mix_Music *Theme;
Mix_Chunk *sound, *intro, *shot, *expsnd, *shield, *crash = NULL;
enum SHIPSTATE ShipState;
int level = 0;
int lives = MAX_LIFE;
int oldX, oldY, oldAngle;
int points = 0;
int currentLevel = 1;
double PlayerShootTime, timetemp = 0;
int expticks = 0;
BOOL explosion = FALSE;
BOOL momentum = FALSE;
BOOL reversed = FALSE;
BOOL shipstill = FALSE;
double velocity = SPEED;
int bestScore = 0;
SCREEN screen;
/* ---------------------------------------------- */
/* FUNCTION PROTOTYPES */
/* ---------------------------------------------- */
/* Mathematics & Physics */
BOOL Collision(int AX1, int AY1, int AX2, int AY2, int BX1, int BY1, int BX2, int BY2);
double sinD(int degree);
double cosD(int degree);
int randnum(int number);
/*SDL Related */
BOOL InitVideo();
BOOL InitAudio();
void ToggleFullscreen(SDL_Window *Window);
void CleanMemory();
/* EVENTS */
BOOL Key(long K);
void HandleKey(long Sym, BOOL Down);
void HandleEvents();
BOOL timer1(int *ticks, double *time, int ms);
BOOL lerp(double *value, double *time, int ms);

/* Game engine */

void LaunchProjectile(double delta, double X, double Y, double DX, double DY, SDL_Surface *Img, int Life);
void LoadAssets();
void Intro();
void NewGame(int currentLevel);
void UpdateGame(double delta);
void Main_Loop();
void LaunchPoof(double delta, int X, int Y, SDL_Surface *Img, int life);
void movePlayerXY(float speed);
void rotateBy(OBJECT *Object, float D);
void Ship_Behaviour();
void moveAsteroids(double delta);
void moveProjectiles(double delta);
void ShootPlayerBullet(float delta, float vel, float rate);
/* Drawing */
void Draw(int X, int Y, SDL_Surface *Img);
void DrawObject(OBJECT Object);
void DrawAnimation(int X, int Y, int H, int W, int frame, SDL_Surface *Img);
void DrawDynamicObject(OBJECT *Object);
void DrawScreen();
void DrawText(char *string, int size, int x, int y, int fR, int fG, int fB, int bR, int bG, int bB, BOOL transparent);
void LoadAsteroids();
void addAsteroid(int X, int Y, int DIRX, int DIRY, int size);

/* ---------------------------------------------- */
/* MAIN - ENTRY POINT */
/* ---------------------------------------------- */
int main()
{
  if (!InitVideo())
    goto end_code;

  InitAudio();
  TTF_Init();
  LoadAssets();
  Intro();
  NewGame(currentLevel);
  SDL_ShowCursor(SDL_DISABLE);
  Main_Loop(); // UPDATE EVENTS AND DRAW TO SCREEN

end_code:
  CleanMemory();
  return 0;
}

/* FUNCTIONS */

/* ==========================================================*/
// Mathematics and Physics
/* ==========================================================*/

BOOL Collision(int AX1, int AY1, int AX2, int AY2, int BX1, int BY1, int BX2, int BY2)
{
  return (AX1 < BX1 + (BX2 - BX1)) && (AX1 + (AX2 - AX1) > BX1) && (AY1 < BY1 + (BY2 - BY1)) && (AY1 + (AY2 - AY1) > BY1);
}
double sinD(int degree)
{
  double ret, val;
  val = PI / 180;
  ret = sin(degree * val);
  return ret;
}
double cosD(int degree)
{
  double ret, val;
  val = PI / 180;
  ret = cos(degree * val);
  return ret;
}
int randnum(int number)
{
  // srand((unsigned) time(&t));
  return rand() % number;
}

/* ==========================================================*/
// SDL Initialization
/* ==========================================================*/

BOOL InitVideo()
{
  if (!get_display_info(&screen))
    return 0;

  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_PNG);

#ifdef __linux__
  win1 = SDL_CreateWindow(" > Ast3r0id <", 0, 0, screen.width, screen.height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN_DESKTOP);
  // SDL_WINDOW_FULLSCREEN_DESKTOP
#elif _WIN32
  win1 = SDL_CreateWindow(" > Ast3r0id <", 50, 50, screen.width, screen.height, SDL_WINDOW_SHOWN);
#else
#endif

  ren1 = SDL_CreateRenderer(win1, -1, SDL_RENDERER_ACCELERATED);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
  SDL_SetWindowBordered(win1, SDL_TRUE);
  // SDL_SetWindowOpacity(win1, 0.6f);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
  return (ren1 != NULL) && (win1 != NULL);
}
BOOL InitAudio()
{
  if (SDL_Init(SDL_INIT_AUDIO) < 0)
  {
    return FALSE;
  }
  Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT,
                MIX_DEFAULT_CHANNELS, 4096);
  return TRUE;
}
void ToggleFullscreen(SDL_Window *Window)
{
  Uint32 FullscreenFlag = SDL_WINDOW_FULLSCREEN;
  BOOL IsFullscreen = SDL_GetWindowFlags(Window) & FullscreenFlag;
  SDL_SetWindowFullscreen(Window, IsFullscreen ? 0 : FullscreenFlag);
  SDL_ShowCursor(IsFullscreen);
}

/* ==========================================================*/
// Events Functions
/* ==========================================================*/

BOOL Key(long K)
{
  if ((K >= 0) && (K <= MAX_KEY))
    return KeyState[K];
  else
    return FALSE;
}
void HandleKey(long Sym, BOOL Down)
{
  if (Sym == SDLK_UP)
    Sym = SDL_SCANCODE_UP;
  if (Sym == SDLK_DOWN)
    Sym = SDL_SCANCODE_DOWN;
  if (Sym == SDLK_LEFT)
    Sym = SDL_SCANCODE_LEFT;
  if (Sym == SDLK_RIGHT)
    Sym = SDL_SCANCODE_RIGHT;
  if (Sym == SDLK_SPACE)
    Sym = SDL_SCANCODE_SPACE;
  if ((Sym >= 0) && (Sym <= MAX_KEY))
  {
    KeyState[Sym] = Down;
    if (Sym == SDLK_ESCAPE)
      Running = FALSE;
  }
}
void HandleEvents()
{
  SDL_Event e;
  if (SDL_PollEvent(&e))
  {
    if (e.type == SDL_QUIT)
    {
      Running = FALSE;
    }

    if (e.type == SDL_KEYDOWN)
    {
      keypressed = TRUE;
      HandleKey(e.key.keysym.sym, TRUE);
    }
    if (e.type == SDL_KEYUP)
    {
      keypressed = FALSE;
      HandleKey(e.key.keysym.sym, FALSE);
    }

    if (e.type == SDL_MOUSEBUTTONDOWN)
    {
      mouseclicked = TRUE;
    }
    if (e.type == SDL_MOUSEBUTTONUP)
    {
      mouseclicked = FALSE;
    }
  }
}

/* ==========================================================*/
// Game Engine
/* ==========================================================*/

void LoadAssets()
{
  /* Images */
  background = SDL_LoadBMP("data/pics/spexpb.bmp");
  if (background == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  asteroid = IMG_Load("data/pics/asteroid.png");
  if (asteroid == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  projectile = IMG_Load("data/pics/fire1.png");
  if (projectile == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  debris = IMG_Load("data/pics/debris.png");
  if (debris == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  indicators = IMG_Load("data/pics/indicators.png");
  if (indicators == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }

  shipSprite[0].Img = IMG_Load("data/pics/ship.png");
  if (shipSprite[0].Img == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  shipSprite[1].Img = IMG_Load("data/pics/ship_plume.png");
  if (shipSprite[1].Img == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  shipSprite[2].Img = IMG_Load("data/pics/ship_plume2.png");
  if (shipSprite[2].Img == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  shipSprite[3].Img = IMG_Load("data/pics/ship_plume3.png");
  if (shipSprite[3].Img == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  shipSprite[4].Img = IMG_Load("data/pics/ship_plume4.png");
  if (shipSprite[4].Img == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  shipSprite[5].Img = IMG_Load("data/pics/ship_plume5.png");
  if (shipSprite[5].Img == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  shipSprite[6].Img = IMG_Load("data/pics/ship_plume6.png");
  if (shipSprite[6].Img == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  shipSprite[7].Img = IMG_Load("data/pics/ship_dmg0.png");
  if (shipSprite[7].Img == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  shipSprite[8].Img = IMG_Load("data/pics/ship_dmg1.png");
  if (shipSprite[8].Img == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }

  ship.Img = shipSprite[0].Img;
  explosionIMG = IMG_Load("data/pics/exp.png");
  if (explosionIMG == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }

  /* Music and Sounds */
  Theme = Mix_LoadMUS("data/snd/theme.ogg");
  if (Theme == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }

  intro = Mix_LoadWAV("data/snd/intro.wav");
  if (intro == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  Mix_VolumeChunk(intro, MIX_MAX_VOLUME);

  sound = Mix_LoadWAV("data/snd/rockets.wav");
  if (sound == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  Mix_VolumeChunk(sound, MIX_MAX_VOLUME);

  shot = Mix_LoadWAV("data/snd/shot.wav");
  if (shot == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  Mix_VolumeChunk(shot, MIX_MAX_VOLUME);

  expsnd = Mix_LoadWAV("data/snd/explosion.wav");
  if (expsnd == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  Mix_VolumeChunk(expsnd, MIX_MAX_VOLUME);

  shield = Mix_LoadWAV("data/snd/shield.wav");
  if (shield == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  Mix_VolumeChunk(shield, MIX_MAX_VOLUME);

  crash = Mix_LoadWAV("data/snd/crash.wav");
  if (crash == NULL)
  {
    fprintf(stderr, ERR_MSG1);
    exit(0);
  }
  Mix_VolumeChunk(crash, MIX_MAX_VOLUME);
}
void LaunchProjectile(double delta, double X, double Y, double DX, double DY, SDL_Surface *Img, int Life)
{
  OBJECT p;
  if (projectiles == NULL)
    p.index = 0;
  else
  {
    p.index = length(&projectiles);
  }
  p.Img = Img;
  p.Angle = ship.Angle;
  p.W = 16;
  p.H = 16;
  if (Life == -1)
  {
    // ship projectile
    p.FX = (ship.X + (ship.W - 10) / 2) + ((cosD(p.Angle) * -1) - (sinD(p.Angle) * 40) * -1);
    p.FY = (ship.Y + (ship.H - 30) / 2) + (sinD(p.Angle) * -1) + ((cosD(p.Angle) * 40 * -1));
    p.DX = DX;
    p.DY = DY;
    p.X = p.FX * delta;
    p.Y = p.FY * delta;
  }
  else
  {
    // poof explosion animation
    p.X = X * delta;
    p.Y = Y * delta;
    p.FX = X;
    p.FY = Y;
    p.DX = DX;
    p.DY = DY;
  }
  p.Life = Life;
  projectiles = addend(projectiles, newelement(p));
}
void addAsteroid(int X, int Y, int DIRX, int DIRY, int size)
{
  OBJECT temp;
  temp.index = length(&asteroids);
  temp.DIRX = DIRX;
  temp.DIRY = DIRY;
  temp.X = X;
  temp.Y = Y;
  temp.DX = temp.X;
  temp.DY = temp.Y;
  if (size == 0)
  {
    temp.W = ASTH0;
    temp.H = ASTW0;
    temp.Life = 3;
  }
  if (size == 1)
  {
    temp.W = ASTH1;
    temp.H = ASTW1;
    temp.Life = 2;
  }
  if (size == 2)
  {
    temp.W = ASTH2;
    temp.H = ASTW2;
    temp.Life = 1;
  }
  temp.Angle = 0;
  temp.Img = asteroid;
  asteroids = addend(asteroids, newelement(temp));
}
void Intro()
{
  char introStr[10];
  char helpStr0[100];
  char helpStr1[100];
  char helpStr2[100];
  char helpStr3[100];
  char helpStr4[100];
  int wherey = 0;
  BOOL keyFlag = FALSE;
  SDL_Event e;

  SDL_RenderClear(ren1);

  // Draw(0, 0, background);
  sprintf(introStr, "AST3R0ID");
  if (Mix_Playing(1) == 0)
    Mix_PlayChannel(1, intro, 0);
  do
  {
    SDL_RenderClear(ren1);
    // Draw(0, 0, background);

    // printf("x: %d, y: %d \n", x, y);

    DrawText(introStr, 100, screen.width / 2 - 240, wherey, 255, 0, 0, 0, 0, 0, TRUE);
    DrawText(introStr, 100, screen.width / 2 - 235, wherey + 5, 255, 255, 0, 0, 0, 0, TRUE);
    SDL_RenderPresent(ren1);
    SDL_Delay(10);
    wherey = wherey + 3;
  } while (wherey < 100);

  int x, y;

  SDL_SetRenderDrawColor(ren1, 255, 0, 0, 255);
  // SDL_Vulkan_GetDrawableSize(win1, &x, &y);
  // SDL_RenderDrawLine(ren1, 0, 0, x, y);
  // SDL_Rect _rect1 = {0, 0, x, y};
  // SDL_RenderDrawRect(ren1, &_rect1);
  sprintf(helpStr0, "Coded by v3l0r3k, enhaced by ilorenz - 2023 ");
  sprintf(helpStr1, "CONTROLS: ");
  sprintf(helpStr2, "^: UP  v: DOWN <- : LEFT  -> : RIGHT | SPACE : SHOOT ");
  sprintf(helpStr3, "ESC: EXIT");
  sprintf(helpStr4, "PRESS ANY KEY TO START");

  DrawText(helpStr0, 12, 230, 200, 105, 105, 105, 0, 0, 0, TRUE);
  DrawText(helpStr1, 12, 10, 370, 255, 255, 255, 0, 0, 0, TRUE);
  DrawText(helpStr2, 12, 10, 390, 255, 255, 255, 0, 0, 0, TRUE);
  DrawText(helpStr3, 12, 10, 410, 255, 255, 255, 0, 0, 0, TRUE);
  DrawText(helpStr4, 16, 200, 250, 255, 255, 255, 0, 0, 0, TRUE);

  SDL_RenderPresent(ren1);

  do
  {
    if (SDL_PollEvent(&e))
    {
      if (e.type == SDL_QUIT)
      {
        Running = FALSE;
        break;
      }
      if (e.type == SDL_KEYDOWN)
      {
        keyFlag = TRUE;
      }
    }

  } while (keyFlag == FALSE);
  SDL_Delay(100);
}
void NewGame(int currentLevel)
{
  int i, a, tDIRX, tDIRY, tSIZE, tX, tY;

  if (asteroids != NULL)
    deleteList(&asteroids);

  lives = MAX_LIFE;
  /* SHIP */
  ship.FX = screen.width / 2 - 30;
  ship.FY = screen.height / 2 - 10;

  ship.DX = 0;
  ship.DY = 0;

  ship.W = 50;
  ship.H = 70;
  ship.Angle = 0;
  oldAngle = ship.Angle;
  shipstill = TRUE;
  /* Asteroids */

  srand((unsigned)time(&t));

  for (i = 0; i < 3 * currentLevel; i++)
  {
    a = rand() % 2;
    if (a == 0)
      tDIRX = 1;
    else
      tDIRX = -1;
    a = rand() % 2;
    if (a == 0)
      tDIRY = 1;
    else
      tDIRY = -1;
    tX = rand() % 640;
    tY = rand() % 480;
    tSIZE = rand() % 3;
    addAsteroid(tX, tY, tDIRX, tDIRY, tSIZE);
  }
  /* Music */
  Mix_PlayMusic(Theme, -1);
}
void CleanMemory()
{
  if (asteroids != NULL)
    deleteList(&asteroids);
  SDL_DestroyRenderer(ren1);
  SDL_DestroyWindow(win1);
  Mix_Quit();
  IMG_Quit();
  TTF_Quit();
  SDL_Quit();
}

/* ==========================================================*/
// Drawing Functions
/* ==========================================================*/

void Draw(int X, int Y, SDL_Surface *Img)
{
  SDL_Rect R;
  SDL_Texture *text;

  R.x = X;
  R.y = Y;
  R.w = Img->w;
  R.h = Img->h;
  text = SDL_CreateTextureFromSurface(ren1, Img);
  SDL_RenderCopy(ren1, text, NULL, &R);
  SDL_DestroyTexture(text);
}
void DrawAnimation(int X, int Y, int H, int W, int frame, SDL_Surface *Img)
{
  SDL_Rect R, D;
  SDL_Texture *text;

  R.x = X;
  R.y = Y;
  R.w = H;
  R.h = W;
  D.x = H * frame;
  D.y = 0;
  D.w = W;
  D.h = W;
  text = SDL_CreateTextureFromSurface(ren1, Img);
  SDL_RenderCopy(ren1, text, &D, &R);
  SDL_DestroyTexture(text);
}
void DrawDynamicObject(OBJECT *Object)
{
  SDL_Rect R;
  SDL_Texture *text;

  R.x = Object->X;
  R.y = Object->Y;
  R.w = Object->W;
  R.h = Object->H;

  text = SDL_CreateTextureFromSurface(ren1, Object->Img);
  SDL_RenderCopyEx(ren1, text, NULL, &R, Object->Angle, NULL, SDL_FLIP_NONE);
  SDL_DestroyTexture(text);
}
void DrawObject(OBJECT Object)
{
  SDL_Rect R;
  SDL_Texture *text;

  R.x = Object.X;
  R.y = Object.Y;
  R.w = Object.W;
  R.h = Object.H;
  text = SDL_CreateTextureFromSurface(ren1, Object.Img);
  SDL_RenderCopyEx(ren1, text, NULL, &R, Object.Angle, NULL, SDL_FLIP_NONE);
  SDL_DestroyTexture(text);
}
void DrawLife()
{
  SDL_Rect rect;
  rect.x = 45;
  rect.y = 3;
  rect.w = lives;
  rect.h = 10;
  if (lives > 0)
  {
    SDL_SetRenderDrawColor(ren1, 255, 69, 0, 0);
    SDL_RenderDrawRect(ren1, &rect);
    SDL_RenderFillRect(ren1, &rect);
  }
  Draw(1, 1, indicators);
}
void DrawText(char *string, int size, int x, int y, int fR, int fG, int fB, int bR, int bG, int bB, BOOL transparent)
{

  // if (TTF_Init == NULL) exit(0);

  TTF_Font *font = TTF_OpenFont("data/fonts/FreeMonoBold.ttf", size);

  SDL_Color foregroundColor = {fR, fG, fB, 0};
  SDL_Color backgroundColor = {bR, bG, bB, 0};
  SDL_Surface *textSurface;

  if (transparent == TRUE)
    textSurface = TTF_RenderText_Blended(font, string, foregroundColor);
  else
    textSurface = TTF_RenderText_Shaded(font, string, foregroundColor, backgroundColor);

  SDL_Texture *texture1 = SDL_CreateTextureFromSurface(ren1, textSurface);

  SDL_Rect textLocation = {x, y, textSurface->w, textSurface->h};

  SDL_RenderCopy(ren1, texture1, NULL, &textLocation);

  SDL_FreeSurface(textSurface);
  SDL_DestroyTexture(texture1);

  TTF_CloseFont(font);
}

/* ==========================================================*/
// FINAL SCREEN COMPOSITION = DRAW SCREEN
/* ==========================================================*/

void DrawScreen(double fps)
{
  int i, a;
  char pointstext[12];
  char levelstr[12];
  char beststr[30];
  char fpsStr[15];

  SDL_SetRenderDrawColor(ren1, 0, 0, 0, 255);
  SDL_RenderClear(ren1);

  switch (ShipState)
  {
  case HALTED:
    ship.Img = shipSprite[0].Img;
    break;
  case UTHRUST:
    a = rand() & 1;
    ship.Img = shipSprite[a + 1].Img;
    break;
  case DTHRUST:
    a = rand() & 1;
    ship.Img = shipSprite[a + 3].Img;
    break;
  case LTHRUST:
    ship.Img = shipSprite[6].Img;
    break;
  case RTHRUST:
    ship.Img = shipSprite[5].Img;
    break;
  case DAMAGED:
    a = rand() & 1;
    ship.Img = shipSprite[a + 7].Img;
    break;
  }
  if (!explosion)
    DrawObject(ship);

  if (asteroids != NULL)
  {
    for (i = 0; i < length(&asteroids); i++)
    {
      DrawDynamicObject(getObject(asteroids, i));
    }
  }

  if (projectiles != NULL && length(&projectiles) > 0)
  {
    for (i = 0; i < length(&projectiles); i++)
    {
      DrawDynamicObject(getObject(projectiles, i));
    }
  }

  if (explosion == TRUE)
  {
    explosion = timer1(&expticks, &timetemp, 100);
    DrawAnimation(ship.X, ship.Y, 60, 60, expticks, explosionIMG);
  }
  DrawLife();
  sprintf(pointstext, "%d", points);
  DrawText(pointstext, 13, 70, 15, 255, 255, 255, 0, 0, 0, TRUE);
  sprintf(levelstr, "LEVEL: %d", currentLevel);
  DrawText(levelstr, 15, 550, 2, 255, 0, 0, 0, 0, 0, TRUE);
  sprintf(beststr, "BEST SCORE: %d", bestScore);
  DrawText(beststr, 15, 280, 5, 255, 255, 255, 0, 0, 0, TRUE);
  sprintf(fpsStr, "FPS: %.2lf", fps);
  DrawText(fpsStr, 15, screen.width - 100, 0, 255, 255, 255, 0, 0, 0, TRUE);
  SDL_RenderPresent(ren1);
}

/* ==========================================================*/
// Move functions - UPDATE GAME
/* ==========================================================*/

void LaunchPoof(double delta, int X, int Y, SDL_Surface *Img, int life)
{
  LaunchProjectile(delta, X, Y, -1, -0.4, Img, life);
  LaunchProjectile(delta, X, Y, -0.2, -0.7, Img, life);
  LaunchProjectile(delta, X, Y, 0.3, -0.6, Img, life);
  LaunchProjectile(delta, X, Y, 0.96, -0.3, Img, life);
  LaunchProjectile(delta, X, Y, -0.8, 0.5, Img, life);
  LaunchProjectile(delta, X, Y, -0.3, 0.65, Img, life);
  LaunchProjectile(delta, X, Y, 0.34, 0.67, Img, life);
  LaunchProjectile(delta, X, Y, 0.93, 0.31, Img, life);
}
void movePlayerXY(float speed)
{
  if (Mix_Playing(1) == 0)
    Mix_PlayChannel(1, sound, 0);

  ship.DX = ship.DX + (speed * sinD(ship.Angle)) * -1;
  ship.DY = ship.DY + (speed * cosD(ship.Angle));
  ship.X += ship.DX;
  ship.Y += ship.DY;
}
void moveProjectiles(double delta)
{
  int i, j;
  OBJECT *p, *a;
  BOOL bcollision = FALSE;
  for (i = 0; i < length(&projectiles); i++)
  {
    p = getObject(projectiles, i);
    // Vector rotation
    // ship projectiles
    if (p->Life == -1)
    {
      p->FX = p->FX + ((p->DX) * delta * sinD(p->Angle));
      p->FY = p->FY + ((p->DY) * delta * cosD(p->Angle)) * -1;
      p->X = p->FX;
      p->Y = p->FY;
    }
    else
    { // poof projectiles
      p->FX = p->FX + (p->DX * delta);
      p->FY = p->FY + (p->DY * delta);
      p->X = p->FX;
      p->Y = p->FY;
    }
    // Delete projectiles which get off the screen
    if (p->Life != -1)
      p->Life = p->Life - 1;
    if (p->Y < -10 || p->Y > screen.height + 10 || p->X < -10 || p->X > screen.width + 10 || p->Life == 0)
    {
      deleteObject(&projectiles, i, TRUE);
      continue;
    }
    // Collision with Asteroids.
    for (j = 0; j < length(&asteroids); j++)
    {
      a = getObject(asteroids, j);
      // Collision with Projectile
      if (Collision(p->X, p->Y, p->X + p->W, p->Y + p->H, a->X, a->Y,
                    a->X + a->W, a->Y + a->H) &&
          p->Life == -1)
      {
        Mix_PlayChannel(5, crash, 0);
        if (a->Life == 1)
        {
          deleteObject(&asteroids, j, TRUE);
          // LaunchPoof(delta, a->X, a->Y, debris, 10);
          if (p->Img != debris)
            points = points + 1;
          continue;
        }
        if (a->Life == 2)
        {
          addAsteroid(a->X, a->Y, 1, 1, 2);
          addAsteroid(a->X, a->Y, -1, -1, 2);
          deleteObject(&asteroids, j, TRUE);
          if (p->Img != debris)
            points = points + 2;
        }
        if (a->Life == 3)
        {
          addAsteroid(a->X, a->Y, 1, 1, 1);
          addAsteroid(a->X, a->Y, -1, -1, 1);
          addAsteroid(a->X, a->Y, 1, -1, 1);
          if (p->Img != debris)
            points = points + 3;
          deleteObject(&asteroids, j, TRUE);
        }
        deleteObject(&projectiles, i, TRUE);
        bcollision = TRUE;
        break;
      }
    }
    if (bcollision == TRUE)
      break;
  }
}
void rotateBy(OBJECT *Object, float D)
{
  float temp;
  if (Mix_Playing(1) == 0)
    Mix_PlayChannel(1, sound, 0);
  if (fabs(Object->Angle + D) < 181)
  {
    temp = Object->Angle + D;
    Object->Angle = temp;
  }
  else
  {
    Object->Angle = Object->Angle * -1;
  }
}
void Ship_Behaviour(double delta)
{
  ship.FX = ship.FX + (ship.DX * delta);
  ship.FY = ship.FY + (ship.DY * delta);
  ship.X = ship.FX;
  ship.Y = ship.FY;

  if (ship.Y < -10)
  {
    ship.Y = screen.height;
    // ship.DY = screen.height;
    ship.FY = screen.height;
  }
  if (ship.Y > screen.height + 10)
  {
    ship.Y = 0;
    // ship.DY = 0;
    ship.FY = 0;
  }
  if (ship.X > screen.width + 10)
  {
    ship.X = 0;
    // ship.DX = 0;
    ship.FX = 0;
  }
  if (ship.X < -10)
  {
    ship.X = screen.width;
    // ship.DX = screen.width;
    ship.FX = screen.width;
  }
}
void moveAsteroids(double delta)
{

  int i, j;
  OBJECT *p, *q;

  for (i = 0; i < length(&asteroids); i++)
  {
    p = getObject(asteroids, i);
    // Collision with Ship
    if (Collision(ship.X + 5, ship.Y + 5, ship.X + ship.W - 5, ship.Y + ship.H - 5, p->X, p->Y,
                  p->X + p->W, p->Y + p->H))
    {
      p->DIRX = p->DIRX * -1;
      p->DIRY = p->DIRY * -1;
      ShipState = DAMAGED;
      lives = lives - 1;
      if (Mix_Playing(4) == 0)
        Mix_PlayChannel(4, shield, 0);
      if (lives == 0)
      {
        // Game Over
        Mix_HaltChannel(-1);
        explosion = TRUE;
        if (bestScore < points)
          bestScore = points;
        points = 0;
      }
    }
    // Collision with Asteroids
    for (j = 0; j < length(&asteroids); j++)
    {
      q = getObject(asteroids, j);
      if (j != i)
      {
        if (Collision(p->X, p->Y + 5, p->X + p->W - 5, p->Y + p->H - 5,
                      q->X, q->Y, q->X + q->W, q->Y + q->H))
        {
          p->DIRX = p->DIRX * -1;
          p->DIRY = p->DIRY * -1;
          p->DIRX = p->DIRX * -1;
          p->DIRY = p->DIRY * -1;
        }
      }
    }
    p->DX = p->DX + (ASTEROID_SPEED * delta * p->DIRX);
    p->DY = p->DY + (ASTEROID_SPEED * delta * p->DIRY);
    p->X = (p->DX);
    p->Y = (p->DY);
    p->Angle += ASTEROID_ROTATION * delta;

    if (p->Y < -10)
    {
      p->Y = screen.height;
      p->DY = screen.height;
    }
    if (p->Y > screen.height + 10)
    {
      p->Y = 0;
      p->DY = 0;
    }
    if (p->X > screen.width + 10)
    {
      p->X = 0;
      p->DX = 0;
    }
    if (p->X < -10)
    {
      p->X = screen.width;
      p->DX = screen.width;
    }
  }
  // LEVEL ACCOMPLISH - NEW GAME
  if (length(&asteroids) == 0)
  {
    // Game Over
    Mix_HaltChannel(-1);
    // delete remaining projectiles
    deleteList(&projectiles);
    SDL_Delay(1000);
    currentLevel++;
    NewGame(currentLevel);
  }
}
void ShootPlayerBullet(float delta, float speed, float rate)
{
  if (((clock() - PlayerShootTime)) * delta > fabs(rate * 1000 * delta))
  {
    PlayerShootTime = clock();
    Mix_PlayChannel(2, shot, 0);
    LaunchProjectile(delta, ship.X + 10, ship.Y - 2, speed + fabs(ship.DX), speed + fabs(ship.DY), projectile, -1);
  }
}

/* ==========================================================*/
// TIMERS
/* ==========================================================*/

BOOL timer1(int *ticks, double *time, int ms)
{
  BOOL value;
  if (SDL_GetTicks() - *time < ms)
  {
    value = TRUE;
    if (Mix_Playing(3) == 0)
      Mix_PlayChannel(3, expsnd, 0);
  }
  else
  {
    *time = SDL_GetTicks();
    if (*ticks < 6)
    {
      *ticks = *ticks + 1;
      value = TRUE;
    }
    else
    {
      *ticks = 0;
      value = FALSE;
      currentLevel = 1;
      NewGame(currentLevel);
    }
  }
  return value;
}
BOOL lerp(double *value, double *time, int ms)
{
  BOOL res;
  if (SDL_GetTicks() - *time < ms)
  {
    res = TRUE;
  }
  else
  {
    *time = SDL_GetTicks();
    if (*value > 0)
    {
      *value = *value - 1;
      res = TRUE;
    }
    else
    {
      *value = 0;
      res = FALSE;
    }
  }
  return res;
}

/* ==========================================================*/
// FINAL UPDATE COMPOSITION
/* ==========================================================*/

void UpdateGame(double delta)
{

  if (Key(SDLK_f))
    ToggleFullscreen(win1);
  if (Key(SDL_SCANCODE_SPACE))
  {
    ShootPlayerBullet(delta, PROJECTILE_SPEED, PROJECTILE_RATE);
  }

  ShipState = HALTED;

  if (Key(SDL_SCANCODE_UP) || Key(SDLK_w))
  {
    ShipState = UTHRUST;
    movePlayerXY(-SPEED * delta);
  }
  if (Key(SDL_SCANCODE_DOWN) || Key(SDLK_s))
  {
    ShipState = DTHRUST;
    movePlayerXY(SPEED * delta);
  }
  if (Key(SDL_SCANCODE_RIGHT) || Key(SDLK_d))
  {
    ShipState = RTHRUST;
    rotateBy(&ship, ROTATION * delta);
  }
  if (Key(SDL_SCANCODE_LEFT) || Key(SDLK_a))
  {
    ShipState = LTHRUST;
    rotateBy(&ship, -ROTATION * delta);
  }

  Ship_Behaviour(delta);
  moveAsteroids(delta);
  moveProjectiles(delta);
}

/* ==========================================================*/
// MAIN LOOP HERE
/* ==========================================================*/

void Main_Loop()
{
  /* Update + HandleEvents - Draw */
  double LastTime, CurrTime, datetime_diff_ms, counterTick, fps = 0;

  LastTime = 0.;
  counterTick = 0;

  while (Running == TRUE)
  {

    HandleEvents();

    CurrTime = clock();
    datetime_diff_ms = (CurrTime - LastTime) / 1000;

    // UpdateGame(datetime_diff_ms / 1000);
    // LastTime = CurrTime;

    if (datetime_diff_ms >= 1000 / FPS_RATE)
    {
      if (counterTick++ > 10)
      {
        fps = 1000 / datetime_diff_ms;
        counterTick = 0;
      }
      UpdateGame(datetime_diff_ms / 1000);
      LastTime = CurrTime;
    }

    DrawScreen(fps);
  }
}
/* ---------------------------------------------- */
/* END 						  */
/* ---------------------------------------------- */
