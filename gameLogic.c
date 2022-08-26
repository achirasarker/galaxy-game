#include "gamelogic.h"

extern actor* player;
extern actor* enemy[3];
extern actor* lasers[4];
extern osThreadId_t animateID;
extern uint8_t lives;
extern uint8_t points;
extern osMutexId_t myMutex;
extern bool meteorShot;
extern bool playerShot;
extern uint8_t meteorShotIndex;
extern uint8_t playerShotIndex;
extern int time;

//sprites for the enemy, player, and laser bolt
char sprite[] = {0x81, 0xA5, 0xFF, 0xE7, 0x7E, 0x24};
char playerSprite[] = {0x18, 0x18, 0xFF, 0x3C, 0x24, 0x24};
char laserSprite[]= {0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
char meteorSprite[] = {0x18, 0x3C, 0x5E, 0x76, 0x3C, 0x18};
char enemySprite[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00} ;

//keep track of runtime
void runtime (void* args)
{
    while (1)
    {
        int i;
        osDelay(1000);
        time ++;

        //check for increments of 10 seconds
        if ((time%10) == 0)
        {
            for (i = 0; i<3; i++)
            {
                //speed up meteors by 2
                lasers[1+i]->speed = lasers[1+i]->speed + 2;
            }
        }
    }
}


void readPlayerInput (void *args)
{
    while(1)
    {
        osMutexAcquire(myMutex, osWaitForever);

        //check the joystick for movement
        if (!(LPC_GPIO1->FIOPIN & (1<<23))) //0 means ON, 1 means OFF
            player->dir = -1;
        else if (!(LPC_GPIO1->FIOPIN & (1<<25)))
            player->dir = 1;
        else
            player->dir = 0;

        //check the button for laser shooting
        if(!(LPC_GPI02->FIOPIN & (1<<10)))
        {
            if(lasers[PLAYER_LASER]->dir == 0)
            {
                //put it in the middle
                lasers[PLAYER_LASER]->horizontalPosition = player->horizontalPosition +
                                                           (SPRITE_ROWS-2)*SPRITE_SCALE/2;
                lasers[PLAYER_LASER]->verticalPosition = player->verticalPosition +
                                                         (SPRITE_COLS-1)*SPRITE_SCALE;
                lasers[PLAYER_LASER]->dir = 1;
            }
        }
        osMutexRelease(myMutex);
    }
}

void enemyShoot (actor* enemy, int i)
{
    //put it in the middle
    //shoot for all enemies
    lasers[i]-›horizontalPosition = enemy->horizontalPosition + (SPRITE ROWS-2) *SPRITE SCALE/2;
    lasers[i]->verticalPosition = enemy-›verticalPosition;
    lasers[i]->dir = -1;
}

void animate (void *args)
{
    while (1)
    {
        int i;
        int j;

        osMutexAcquire(myMutex, osWaitForever);

        //print all three enemies
        for (i = 0; i<3; i++)
            printEnemy(enemy[i]);

        //see if there is shooting to do
        //shoot for all three enemies
        if (rand() > ENEMY_SHOT_CHANCE && lasers [ENEMY_LASER1]->dir == 0)
            enemyShoot(enemy[ENEMY_LASER1-1], ENEMY_LASER1);

        if (rand () > ENEMY_SHOT_CHANCE && lasers[ENEMY_LASER2]->dir == 0)
            enemyShoot (enemy [ENEMY_LASER2-1] , ENEMY_LASER2);

        if (rand() > ENEMY_SHOT_CHANCE && lasers (ENEMY_LASER3] ->dir == 0)
        enemyShoot(enemy[ENEMY_LASER3-1], ENEMY_LASER3);

        if(player->dir != 0) //we don't want to print more than we need. The print logic is a bit more complex than for enemies
            printPlayer(player);

        if(lasers[PLAYER_LASER]->dir != 0)
            printlaser(lasers[PLAYER_LASER]);

        for (j = 1; j<4; j++)
        {
            if (lasers[j]->dir != 0)
                printlaser(lasers[j]);
        }

        //cover up meteor sprite if player gets hit with meteor
        if (playerShot)
        {
            GLCD_SetTextColor(Black);

            printSpriteAt(lasers[playerShotIndex]->prevVertical, lasers[playerShotIndex]->prevHorizontal, lasers(playerShotIndex]->sprite);
            playerShot = false;
        }

        //cover up meteor sprite if meteor gets hit by player laser
        if (meteorShot)
        {
            GLCD_SetTextColor(Black);

            printSpriteAt(lasers[meteorShotIndex]->prevVertical, lasers[meteorShotIndex]->prevHorizontal, lasers[meteorShotIndex]-›sprite);
            meteorShot = false;
        }

        osDelay(100U);
        osMutexRelease(myMutex);
    }
}

//thread to check for end-game conditions
void checkEndGame (void* args)
{
    while(1)
    {
        int i;
        osMutexAcquire(myMutex, osWaitForever);

        for (i = 0; i<3; i++)
        {
            if (checkCollision (player, lasers[1+i], 1))
            {
                //update meteor previous position
                lasers[1+i]->prevHorizontal = lasers[1+i]->horizontalPosition;
                lasers[1+i]->prevVertical = lasers[1+i]->verticalPosition;

                //update global bool
                playerShot = true;
                playerShotIndex = 1+i;

                //decrement lives and print sad message to console lives
                lives--;
                printf ("\nYou got hit : ( Lives: %d", lives);

                //display lives on lights
                displayLives(lives);

                //reset meteor position
                lasers[1+i]->verticalPosition = enemy[i]->verticalPosition;
                lasers[1+i]->horizontalPosition = enemy[i]->horizontalPosition;

                //end game if player runs out of lives
                if (lives == 0)
                {
                    osThreadTerminate(animateID);
                }
            }
        }
        osMutexRelease(myMutex);
    }
}

//points: if player laser hits a meteor
void checkPoints (void* args)
{
    while(1)
    {
        int i;
        osMutexAcquire(myMutex, osWaitForever);

        for (i = 0; i<3; i++)
        {
            if (checkCollision (lasers[1+i], lasers[PLAYER LASER], 1) )
            {
                //update meteor previous position
                lasers[1+i]->prevHorizontal = lasers[1+i]->horizontalPosition;
                lasers[1+i]->prevVertical = lasers[1+i]->verticalPosition;

                //update global bool
                meteorShot = true;
                meteorShotIndex = 1+i;

                //reset meteor position
                lasers[1+i]->verticalPosition = enemy[i]->verticalPosition;
                lasers[1+i]->horizontalPosition = enemy[i]->horizontalPosition;

                //increment points and print encouraging message
                points ++;
                printf("\nNice Shot! Points: %d", points);
            }
        }
        osMutexRelease(myMutex);
    }
}

void initializeActors()
{
    /*
        Generally this should be handled by a separate function. However, it made sense for simplicity of understanding the code that the first thing we do is initialize the actors, so we did this in main instead.
    */

    int i;

    //create three enemies and three meteor “lasers”
    for (i = 0; i<3; i++)
    {
        enemy[i] = malloc (sizeof(actor));
        enemy[i]->horizontalPosition = 20 + 70*i;
        enemy[i]->verticalPosition = 300;
        enemy[i]->speed = ENEMY_SPEED;
        enemy[i]->dir = 1;
        enemy[i]->sprite = enemySprite;

        lasers[1+i] = malloc (sizeof(actor));
        lasers[1+i]->horizontalPosition = 0;
        lasers[1+i]->verticalPosition = 0;
        lasers[1+i]->speed = ENEMY_LASER_SPEED + 2*i;
        lasers[1+i]->dir = 0;
        lasers[1+i]->sprite = meteorSprite;
    }

    player = malloc(sizeof(actor));
    player->horizontalPosition = 112; //start in middle of screen
    player->verticalPosition = 20;
    player->speed = PLAYER_SPEED;
    player->dir = 0; //set to zero until the player moves
    player->sprite = playerSprite;

    //init the two lasers, but set their dir to zero so they don't appear or affect anything
    lasers[PLAYER_LASER] = malloc (sizeof(actor));
    lasers[PLAYER_LASER]->horizontalPosition = 241;
    lasers[PLAYER_LASER]->verticalPosition = 321;
    lasers[PLAYER_LASER]->speed = LASER_SPEED;
    lasers[PLAYER_LASER]->dir = 0;
    lasers[PLAYER_LASER]->sprite = laserSprite;
}