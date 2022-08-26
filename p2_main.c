#include <lpc17xx.h>
#include <stdlib.h>
#include <stdio.h>
#include "GLCD.h"
#include "spece.h"
#include <cmsis os2.h>
#include <os tick.h>
#include "gamelogic.h"

osMutexId_t myMutex;
void readPlayerInput(void *args);
void animate(void *args);
void checkEndGame(void* args);
void checkPoints(void* args);
void runtime(void* args);

const osMutexAttr t myMtxp = {
        "myButtonMutex",
        0,
        NULL,
        0U
};

actor* enemy[3];
actor* player;
actor* lasers[4]; //an array of actor pointers!
uint8_t lives = 3;
uint8_t points = 0;
bool meteorShot = false; //track if a meteor gets shot by player
bool playerShot = false; //track if a player gets hit by meteor
uint8_t meteorShotIndex; //index of meteor when meteor gets shot
uint8_t playerShotIndex; //index of meteor when player gets hit
uint16_t time = 0;

/*
    we need to be able to stop the animation thread once one of the actors dies, so
    we are keeping its ID global so that other threads checking on it can stop it
*/
osThreadId_t animateID;

//display the remaining number of lives on the LEDs
void displaylives (unsigned int n)
{
    int i;

    //use LED pins 4, 5, 6
    //clear LEDs
    for (i = 4; i<= 6; i++)
        LPC_GPIO2->FIOCLR |= 1<<i;

    //turn on LEDs according to number of lights
    for(i = n; i >= 1; i--)
        LPC_GPIO2->FIOSET |= 1<<(i+3);
}

int main()
{
    //set up the main actors in the game
    initializeActors();

    //call this function to ensure that all of the internal settings of the LPC are correct
    SystemInit();

    //set up mutex
    myMutex = OsMutexNew(&myMtxp);

    //set LED pin directions
    LPC_GPIO1->FIODIR |= 1<<28; // LED 7
    LPC_GPIO1->FIODIR |= 1<<29;
    LPC_GPIO1->FIODIR |= 1<<31;
    LPC_GPIO2->FIODIR |= 1<<2;
    LPC_GPIO2->FIODIR |= 1<<3;
    LPC_GPIO2->FIODIR |= 1<<4;
    LPC_GPIO2->FIODIR |= 1<<5;
    LPC_GPIO2->FIODIR |= 1<<6; // LED 0

    //initialize LEDs to display 3 lives
    displayLives(lives);

    printf ("\nGAME BEGINS");

    /*
        Initialize the LCD screen, clear it so that it is mostly black, then set the "text" color to green.
        By setting the text color to green, every pixel we print is also going to be green
    */
    GLCD_Init();
    GLCD_Clear(Black);
    GLCD_SetTextColor(Green); //actually sets the color of any pixels we directly write to the screen, not just text

    /*
        For efficiency reasons, the player character only gets re-drawn if it has moved,
        so we need to print it before the game starts or we won't see it
    */
    printPlayer(player);

    //initialize the kernel so that we can create threads
    osKernelInitialize();

    //create a new thread for animate and store its ID so that we can shut it down at the endgame
    animateID = osThreadNew(animate, NULL, NULL);

    //create a new thread for reading player input
    osThreadNew(readPlayerInput, NULL, NULL);

    //create a new thread that checks for endgame
    osThreadNew(checkEndGame, NULL, NULL);

    //create a new thread that checks for endgame
    osThreadNew(checkPoints, NULL, NULL);

    //create a new thread that keeos track of runtime
    osThreadNew(runtime, NULL, NULL);

    //launch the kernel, which simultaneously starts all threads we have created
    osKernelStart();

    //Theoretically we will only ever entire this loop if something goes wrong in the kernel
    while(1){};
}

