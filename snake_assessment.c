#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "graphics.h"
#include "sprite.h"
#include "cpu_speed.h"
#include "byte.h"
#include <stdio.h>

//Variable Declaration
int gameOver;
int score = 0;
int lives = 5;
int walls = 0;
int snakeSize;
int direction;
Sprite snake[30];
Sprite food;

//Bitmaps
static byte snake_char[] = {
	BYTE(11111111),
	BYTE(11111111),
	BYTE(11111111)
};

static byte food_char[] = {
	BYTE(01000000),
	BYTE(11111111),
	BYTE(01000000)
};

//Initialise hardware
void InitHardware(void) {
	//Set clock speed
	set_clock_speed(CPU_8MHz);

	//Set seed for random value generation
	srand(TCNT0);

	//Initialise LCD screen
	lcd_init(LCD_DEFAULT_CONTRAST);

	//Set Timer0 and Timer1 to normal mode
	TCCR0B &= ~(1 << WGM02);
	TCCR1B &= ~(1 << WGM02);

	//Enable interrupts
	TIMSK0 |= (1 << TOIE0);

	//Configure Timer0 and Timer1 prescaler
	TCCR0B |= (1 << CS02) | (1 << CS00);
	TCCR0B &= ~(1 << CS01);

	TCCR1B |= 1 << CS00 | 1 << CS02;
	TCCR1B &= ~(1 << CS01);

	//LCD Backlight
	PORTC |= 0b10000000;

	//Setup adc
	ADMUX |= 1 << REFS0;
	ADCSRA |= 1 << ADEN | 1 << ADPS2 | 1 << ADPS1 | 1 << ADPS0; 

	//Globally enable interrupts
	sei();
}

//Initial screen
void IntroScreen(void) {
	draw_string(5, 20, "Tyler McKerihan");
	draw_string(20, 30, "N9447482");
	show_screen();
	_delay_ms(2000);
	clear_screen();
}

//Reset the food's position
void ResetFood(void) {
	int randX = 0;
	int randY = 0;
	srand(TCNT0);

	for (int i = 0; i < snakeSize; i++) {
		do {
			randX = ((rand() % (75/3))  * 3);
			randY = ((rand() % (48/3)) * 3);
		} while (randX > 73 || randY > 37 ||
			randY < 1 || (randX == snake[i].x + 2 &&
			randY == snake[i].y + 8));
	}

	food.x = randX + 2;
	food.y = randY + 8;

	draw_sprite(&food);
}

//Setup teensy
void setup(void) {
	//Set initial variable values
	gameOver = 0;
	snakeSize = 2;
	direction = 4;
	init_sprite(&snake[0], 20, 20, 3, 3, snake_char);
	init_sprite(&food, 0, 0, 3, 3, food_char);

	//Initial food placement
	ResetFood();
}

//Setup environment
void EnvironmentSetup() {
	//Setup statusbar
	char statusBar[60];
	sprintf(statusBar, "%d(%d)", lives, score);
	draw_string(5, 0, statusBar);
}

//Update snake location
void MoveSnake() {
	//Direction:
	//   0
	//3     1
	//   2
	
	draw_sprite(&snake[0]);
	
	if (direction == 0) {
	init_sprite(&snake[0], snake[0].x, snake[0].y - 3, 3, 3, snake_char);
	draw_sprite(&snake[0]);
	} else if (direction == 1) {
	init_sprite(&snake[0], snake[0].x + 3, snake[0].y, 3, 3, snake_char);
	draw_sprite(&snake[0]);
	} else if (direction == 2) {
	init_sprite(&snake[0], snake[0].x, snake[0].y + 3, 3, 3, snake_char);
	draw_sprite(&snake[0]);
	} else if (direction == 3) {
	init_sprite(&snake[0], snake[0].x - 3, snake[0].y, 3, 3, snake_char);
	draw_sprite(&snake[0]);
	}

	for (int i = snakeSize; i != 0; i--) {
		init_sprite(&snake[i], -5, -5, 3, 3, snake_char);
		snake[i].x = snake[i - 1].x;
		snake[i].y = snake[i - 1].y;
		draw_sprite(&snake[i]);
	}
}

//Reset snake when life lost
void ResetSnake(void) {
	snakeSize = 2;
	init_sprite(&snake[0], 20, 20, 3, 3, snake_char);
	direction = 4;
	lives--;
}


//Check for collisions
void CollisionCheck(void) {
	//Snake and food
	if (snake[0].x == food.x &&
	    snake[0].y == food.y) {
		if (walls == 0) {
			score++;
		} else if (walls == 1) {
			score += 2;
		}
		snakeSize++;
		ResetFood();
	}

	//Snake and snake
	for (int i = 2; i < snakeSize + 1; i++) {
		if (snake[0].x == snake[i].x &&
		    snake[0].y == snake[i].y &&
		    direction != 4) {
			ResetSnake();
		}
	}
}

//Check for barrier collision
void BarrierCollision(void) {
	if (snake[0].x > 82) {
		snake[0].x = -1;
	} else if (snake[0].x < 0) {
		snake[0].x = 83;
	} else if (snake[0].y > 46) {
		snake[0].y = 8;
	} else if (snake[0].y < 8) {
		snake[0].y = 47;
	}
}

//Draw walls
void DrawWalls(void) {
	draw_line(14, 8, 14, 23);
	draw_line(59, 8, 59, 23);
	draw_line(32, 48, 32, 24);
}

//Wall collision check
void WallCollision(void) {
	//Snake and wall
	for (int i = 0; i < snakeSize; i += 2) {
		//Left wall
		if (snake[i].x == 14 &&
				(snake[i].y > 7 && 
				 snake[i].y < 23)) {
			ResetSnake();
			//Middle wall
		} else if (snake[i].x == 32 &&
				(snake[i].y > 23 &&
				 snake[i].y < 49)) {
			ResetSnake();
			//Right wall
		} else if (snake[i].x == 59 &&
				(snake[i].y > 7 &&
				 snake[i].y < 24)) {
			ResetSnake();
		}
	} 

	//Food and wall
	if (food.x > 12 &&
	    food.x < 16 &&
	   (food.y > 7 &&
    	    food.y < 49)) {
		ResetFood();
	} else if (food.x > 30 &&
		   food.x < 34 &&
		  (food.y > 23 &&
	   	   food.y < 49)) {
     		ResetFood();
	} else if (food.x > 57 &&
		   food.x < 61 &&
		  (food.y > 7 &&
   		   food.y < 24)) {
		ResetFood();
	}		
}

//General process function
void process(void) {
	draw_sprite(&food);
	MoveSnake();
	CollisionCheck();
	BarrierCollision();
	if (walls == 1) {
		DrawWalls();
		WallCollision();
	}
}

//Return adc as reasonable delay figure
int16_t read_adc(int ch) {
	ADMUX &= 0b11111000;
	ADMUX |= (ch & ~(0xF8));
	ADCSRA |=  1 << ADSC;
	while (ADCSRA & (1 << ADSC));

	return ADC/3+10;
}

//Personalised dynamic delay
void myDelay(int n) {
	while(n--) {
		_delay_ms(1);
	}
}

//Game over screen
void GameOver(void) {
	clear_screen();
	draw_string(18, 20, "Game Over!");
	show_screen();
}

int main() {
	InitHardware();
	IntroScreen();
	show_screen();
	setup();

	while (gameOver == 0) { 
		clear_screen();
		EnvironmentSetup();
		process();

		int adc = read_adc(1);
		myDelay(adc);	
		show_screen();
		if (lives == 0) {
			gameOver = 1;
		}
	}

	GameOver();

	return 0;
}

//Timer0 interrupt
ISR(TIMER0_OVF_vect) {
	int tempDirection = direction;
	//Joystick left
	if ((PINB >> 1) & 0b1) {
		if (direction != 3) {
			_delay_ms(50);
			while (((PINB >> 1) & 0b1));
			_delay_ms(50);
		}
		tempDirection = direction;
		direction = 3;
	//Joystick right
	} else if ((PIND >> 0) & 0b1) {
		if (direction != 1) {
			_delay_ms(50);
			while (((PIND >> 0) & 0b1));
			_delay_ms(50);
		}
		tempDirection = direction;
		direction = 1;
	//Joystick up
	} else if ((PIND >> 1) & 0b1) {
		if (direction != 0) {
			_delay_ms(50);
			while (((PIND >> 1) & 0b1));
			_delay_ms(50);
		}
		tempDirection = direction;
		direction = 0;
	//Joystick down
	} else if ((PINB >> 7) & 0b1) {
		if (direction != 2) {
			_delay_ms(50);
			while (((PINB >> 7) & 0b1));
			_delay_ms(50);
		}
		tempDirection = direction;
		direction = 2;
	//Right button
	} else if ((PINF >> 5) & 0b1) {
		_delay_ms(50);
		while(((PINF >> 5) & 0b1));
		_delay_ms(50);
		walls = 1;
	//Left button
	} else if ((PINF >> 6) & 0b1) {
		_delay_ms(50);
		while(((PINF >> 5) & 0b1));
		_delay_ms(50);
		walls = 0;
	}

	//Check for snake going back on itself
	if ((tempDirection == 0 && direction == 2) ||
	    (tempDirection == 2 && direction == 0) ||
	    (tempDirection == 1 && direction == 3) ||
	    (tempDirection == 3 && direction == 1)) {
		ResetSnake();
	}
}
