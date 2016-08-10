// bonus1 - by Maor Halpern 303082887
// and Omri Couri 200608248

#include <dos.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>


// colors:

#define Magenta 5
#define Green 2
#define Yellow 14
#define Red 4
#define Blue 1

// no. of pawns:

#define m_num 10
#define g_num 4
#define r_num 4

//pawn struct:

typedef struct
{
	int x;
	int y;
} Pawn;

// global variables:

volatile Pawn *pacman;
volatile Pawn *magentas;
volatile Pawn *greens;
volatile Pawn *reds;
volatile int table[150][150] = { 0 }; 	// stores table pawns data
volatile int counter = 0; 		// counts time since first continued keystroke
volatile int weak_state_counter = 0;
volatile int weak_state_time = 0;
volatile int diff = 0; 
volatile int ghost_color = 4;
volatile int purple_pills_eaten = 0;
volatile int enter_pressed = 0;

// func/interrupts:

void set_screen_on();
void put_pawns( int, int );
void start_game();
void move_pawn ( int, int, Pawn *temp_pawn, int );
void color_pixel( int , int , int );
void game_over();
void you_won();
void move_reds();
int check_if_possible( int, int, int );
void finish_game( int );
void Kill_red ( int, int );
void set_screen_off();

void interrupt myint9 (void); // declaration of my int 9
void interrupt (*int9save) (void); // saves the original int 9

void interrupt myint8 (void); // declaration of my int 8
void interrupt (*int8save) (void); // saves the original int 8


// main stuff:


void finish_game( int state )
{ 
//when game is finished, show message and exit to dos

	//change ints back to normal

	setvect( 9, int9save );
	setvect( 8, int8save );

	//check if player won or lost
	if ( state )
		you_won( );
	else game_over( );

	sleep(3);

	//close the display and exit back to dos
	set_screen_off();
	asm{
		MOV AH, 4ch
		INT 21h
	}

} //end of finish_game


void kill_red( int x, int y )
{
/*each time pac-man eats a red pawn
erase it from board*/

	//find the red pawn
	int i;
	for ( i = 0 ; i < r_num ; i ++ )
	{
		if ( ( reds[i].x == x ) && ( reds[i].y == y ) )
		{
		//set its place as (-1,-1)
			reds[i].x = -1;
			reds[i].y = -1;
			return;
		}

	} //end of for

} //end of kill_red


void move_reds()
{
//every 19/diff clock cycles move the red pawns

	int i, temp_rand = 0, ok = 0;

	//for each red pawn
	for ( i = 0 ; i < r_num ; i ++ )
	{	
		//check if he is in game
		if ( reds[i].x != -1 )
		{
			//as long as the pawn can't move..
			while ( !ok )
			{
				//randomaly pick a direction
				temp_rand = ( rand() % 4 );	
				

				/*if pawn can move in direction then..
				stop the ok while-loop.
				and move the pawn */

				switch( temp_rand )
				{	
					case 0 :	//down arrow:

						if ( check_if_possible( reds[i].x, (reds[i].y+1), ghost_color ) )
						{
							ok++;
							move_pawn ( 0, 1, &reds[i], ghost_color );
						}
						break;

					case 1 : 	// left arrow:

						if ( check_if_possible( (reds[i].x-1), reds[i].y, ghost_color ) )
						{
							ok++;
							move_pawn ( -1, 0, &reds[i], ghost_color );
						}
						break;

					case 2 : 	// right arrow:

						if ( check_if_possible( (reds[i].x+1), reds[i].y, ghost_color ) )
						{
							ok++;
							move_pawn ( 1, 0, &reds[i], ghost_color );
						}
						break;

					default : 	// up arrow:

						if ( check_if_possible( reds[i].x, (reds[i].y-1), ghost_color ) )
						{
							ok++;
							move_pawn ( 0, -1, &reds[i], ghost_color );
						}

				} //end of switch

			} //end of while

		}  //end of if

		ok = 0; //sets ok for next pawn

	} //end of for

} //end of move_reds


void interrupt myint8()
{
/*uses int8 to move red pawns
and allow user to enter a killing state
in which he has limited time to kill
red pawns*/

	//calls the original int8
	asm{
		PUSHF
		CALL DWORD PTR int8save
	}

	//if it's killing state..
	if ( enter_pressed )
	{
		if ( ghost_color == Blue )
		{
			//count the time of killing state that past
			weak_state_counter++;

			//if killing state is over then..
			if ( ( weak_state_counter ) >= ( weak_state_time ) )
			{

				//return red pawns to their normal color
				ghost_color = Red;
	
				//and reset the counter
				weak_state_counter = 0;

			}
		} //end of if
	

		/*anyway inc the counter, and in case of
		it's time to move red pawns, then move them*/

		counter++;
		if ( counter == ( ceil(19/diff) ) )
		{
			move_reds();

			//start counting again
			counter = 0;
		}

	} //end of outer if

} // end of myint8


void interrupt myint9()
{
// uses int9 to allow user to move pac-man

	int scan_code;
	int ascii_code;
	Pawn temp = *pacman;

	/* calls original int 9 and get the key that been pressed
	then clear it from the keyboard type-ahead buffer*/

	asm{
		PUSHF
		CALL DWORD PTR int9save
		PUSH AX
		XOR AX,AX
		INT 16h
		MOV BYTE PTR scan_code, AH
		MOV BYTE PTR ascii_code, AL
		POP AX
	}

	if ( !enter_pressed )
		if ( ascii_code == 13 )
			enter_pressed = 1;
	

	/*if pac-man can move in direction then..
	move it in the direction that have been pressed */

	if ( enter_pressed )
	{
		switch( scan_code )
		{
			case 0x50 : 	// down arrow
				if ( check_if_possible( temp.x, (temp.y + 1), Yellow ) )
					move_pawn ( 0, 1, pacman, Yellow );
				break;
			case 0x4B : 	// left arrow
				if ( check_if_possible( (temp.x - 1), temp.y, Yellow ) )
					move_pawn ( -1, 0, pacman, Yellow );
				break;
			case 0x4D : 	// right arrow
				if ( check_if_possible((temp.x + 1), temp.y, Yellow ) )
					move_pawn ( 1, 0, pacman, Yellow );
				break;
			case 0x48 : 	// up arrow
				if ( check_if_possible( temp.x, (temp.y - 1), Yellow ) )
					move_pawn ( 0, -1, pacman, Yellow );
				break;
		} //end of switch

	} //end of if

} //end of myint9


int check_if_possible( int new_x, int new_y, int color )
{
/*check whether pac-man or red pawn can move to
the tile placed in board point (new_x, new_y)
if so return 1 so it will move, else return 0 so it won't move */

	int temp;
	
	/* why '>', '<' ? because if the tile the pawn or pac-man
	would like to go is beyond the board, it's illegal
	and it also means that the pawn is at the edge */

	if ( ( new_x > 199 ) || ( new_y > 199 ) || ( new_x < 50 ) || ( new_y < 50 ) )
		return 0;

	temp =  table[new_x-50][new_y-50];

	// what to do if the pawn is red or in kill state:

	// in kill state, it can't eat any color
	if ( color == Blue )
		if ( temp )
			return 0;

	// if not in kill state, can only eat pac-man
	if ( color == Red )
		if ( ( temp == Magenta ) || ( temp == Green ) )
			return 0;
	
	return 1;
} //end of check_if_possible


void move_pawn( int x_dir, int y_dir, Pawn *temp_pawn, int color )
{
//what to do when a pawn moves

	int temp_x = (*temp_pawn).x, temp_y = (*temp_pawn).y;

	//the tile is no longer with color:

	table[temp_x-50][temp_y-50] = 0;
	color_pixel( temp_x, temp_y, 0 );
	

	temp_x += x_dir;
	temp_y += y_dir;
	
	// if it's pac-man, then..
	if ( color == Yellow)
	{
		// ..if he eats a purple pill..
		if ( table[temp_x-50][temp_y-50] == Magenta )
		{
			/* add 1 to purple pills eaten counter
			if that counter reaches the number of
			purple pills on the board, player wins*/

			purple_pills_eaten++;

			if ( purple_pills_eaten == m_num )
				finish_game( 1 );

			goto Label1;

		}

		// ..if he eats a green pill..
		if ( table[temp_x-50][temp_y-50] == Green )
		{
			// enter kill state:

			ghost_color = 1;
			weak_state_counter = 0;

			goto Label1;

		}

		// ..if he tries to eat a red pawn, he dies..
		if ( table[temp_x-50][temp_y-50] == Red )
		{
			finish_game( 0 );

			goto Label1;
		}
		
		/* ..if he tries to eat a red pawn while in 
		kill state, make sure the pawn is dead */

		if ( table[temp_x-50][temp_y-50] == Blue )
			kill_red( temp_x, temp_y );
	} //end of if

	
	// if pawn is red and he eats pac-man, player lost:

	if ( color == Red )
		if ( table[temp_x-50][temp_y-50] == Yellow )
			finish_game( 0 );
		
	// either way, change tile color to pawn color:

Label1: table[temp_x-50][temp_y-50] = color; 
	color_pixel( temp_x, temp_y, color);

	// change pawn location:
	(*temp_pawn).x = temp_x;
	(*temp_pawn).y = temp_y;

} //end of move_pawn 


void start_game()
{
//set new vectors and start the game

	int9save = getvect( 9 );
	setvect( 9, myint9 );
	int8save = getvect( 8 );
	setvect( 8, myint8 );
	while(1);
	

} //end of start_game



void pixel_pacman(int difficulty)
{
//initialize the game board
	diff = difficulty;
	weak_state_time = (10-difficulty)*18.2;
	set_screen_on();			//sets a game table
	srand(time(NULL));			
	put_pawns( m_num, Magenta );	//sets magenta pills
	put_pawns( g_num, Green );		//sets green pills
	put_pawns( r_num, Red );		//sets red pills
	put_pawns( 1, Yellow );		//sets a pacman
	start_game();

} //end of pixel_pacman

void set_screen_on()
{
//turn on the graphic for displaying the pixels

	asm{
		PUSH AX		//; Preserve AX
		MOV AH,0	//; Set screen...
		MOV AL,13h	//; ...for displaying...
		INT 10h		//; ...the board
		POP AX		//; Restore AX
	}

} //end of set_screen_on


void set_screen_off()
{
//turn off the graphic for displaying the pixels

		asm{
		PUSH AX		//; Preserve AX
		MOV AX,3	//; Set command...
		INT 10h		//; ... to close the display
		POP AX		//; Restore AX
	}

} //end of set_screen_off


void color_pixel( int x, int y, int color )
{
//colors a single pixel in location (x,y)

	asm{
	PUSH AX
	PUSH CX
	PUSH DX
	MOV AH, 0ch
	MOV AL, BYTE PTR color
	MOV CX, x
	MOV DX, y
	INT 10h
	POP DX
	POP CX
	POP AX
	}

} //end of color_pixel

void put_pawns( int times, int color )
{
//put all of the same color pawns randomaly on the board

	int row, colum, i, ok;
	Pawn *new_pawn, *temp_pawn;
	
	//will be used to store data about group of pawns
	new_pawn = (Pawn *)malloc(times * sizeof(Pawn));

	i = 0;


	//runs as much as the amount of same colored pawns:
	while ( i < times )
	{
		
		ok = 0;

		//while the place is taken
		while( !ok )
		{
			row = ( rand() % 150 );
			colum = ( rand() % 150 );
			if ( table[row][colum] == 0 )
				ok++;
		}
		
		//set location of the new pawn:

		table[row][colum] = color ;
		new_pawn[i].x = row + 50;
		new_pawn[i++].y = colum + 50;
		color_pixel( row + 50, colum + 50, color);

	} //end of outer while
	
	switch( color )
	{
		case Magenta :
			magentas = new_pawn;
			break;
		case Green :
			greens = new_pawn;
			break;
		case Red :
			reds = new_pawn;
			break;
		default :
			pacman = new_pawn;
	}

} //end of put_pawns


// main as posted in bonus_hw1 instructions
//can be used either with while and without it

void main(void)
 {
	int ok = 0;;

	printf("in order to start moving press <Enter>...\n\n");

	while ( ( ok != 1 ) && ( ok != 2 ) && ( ok != 3 ) )
	{
		printf("Please enter difficulty for game\n 1, 2 or 3 only!!\n> ");
		scanf("%d", &ok);
	}

	pixel_pacman (2);

 } /* main */
// end of bonus1.

//this is only for printing the messages in a nicer way:

void game_over()
{
	//print g

	color_pixel(75, 150, Red);
	color_pixel(75, 151, Red);
	color_pixel(75, 152, Red);
	color_pixel(75, 154, Red);

	color_pixel(76, 150, Red);
	color_pixel(76, 152, Red);
	color_pixel(76, 154, Red);

	color_pixel(77, 150, Red);
	color_pixel(77, 151, Red);
	color_pixel(77, 152, Red);
	color_pixel(77, 153, Red);
	color_pixel(77, 154, Red);

	//print a

	color_pixel(79, 150, Red);
	color_pixel(79, 151, Red);
	color_pixel(79, 152, Red);
	color_pixel(79, 153, Red);

	color_pixel(80, 150, Red);
	color_pixel(80, 153, Red);

	color_pixel(81, 150, Red);
	color_pixel(81, 151, Red);
	color_pixel(81, 152, Red);
	color_pixel(81, 153, Red);
	color_pixel(81, 154, Red);

	//print m

	color_pixel(83, 151, Red);
	color_pixel(83, 152, Red);
	color_pixel(83, 153, Red);
	color_pixel(83, 154, Red);

	color_pixel(84, 150, Red);
	color_pixel(86, 150, Red);

	color_pixel(85, 151, Red);
	color_pixel(85, 152, Red);
	color_pixel(85, 153, Red);
	color_pixel(85, 154, Red);

	color_pixel(87, 151, Red);
	color_pixel(87, 152, Red);
	color_pixel(87, 153, Red);
	color_pixel(87, 154, Red);

	//print e

	color_pixel(89, 151, Red);
	color_pixel(89, 152, Red);
	color_pixel(89, 153, Red);

	color_pixel(90, 150, Red);
	color_pixel(90, 152, Red);
	color_pixel(90, 154, Red);

	color_pixel(91, 150, Red);
	color_pixel(91, 152, Red);
	color_pixel(91, 154, Red);

	color_pixel(92, 151, Red);
	color_pixel(92, 154, Red);

	//print o

	color_pixel(96, 150, Red);
	color_pixel(96, 151, Red);
	color_pixel(96, 152, Red);
	color_pixel(96, 153, Red);
	color_pixel(96, 154, Red);

	color_pixel(99, 150, Red);
	color_pixel(99, 151, Red);
	color_pixel(99, 152, Red);
	color_pixel(99, 153, Red);
	color_pixel(99, 154, Red);

	color_pixel(98, 150, Red);
	color_pixel(98, 154, Red);

	color_pixel(97, 150, Red);
	color_pixel(97, 154, Red);

	//print v

	color_pixel(101, 150, Red);
	color_pixel(101, 151, Red);
	color_pixel(101, 152, Red);

	color_pixel(105, 150, Red);
	color_pixel(105, 151, Red);
	color_pixel(105, 152, Red);

	color_pixel(102, 153, Red);
	color_pixel(103, 154, Red);
	color_pixel(104, 153, Red);

	//print e

	color_pixel(107, 151, Red);
	color_pixel(107, 152, Red);
	color_pixel(107, 153, Red);

	color_pixel(108, 150, Red);
	color_pixel(108, 152, Red);
	color_pixel(108, 154, Red);

	color_pixel(109, 150, Red);
	color_pixel(109, 152, Red);
	color_pixel(109, 154, Red);

	color_pixel(110, 151, Red);
	color_pixel(110, 154, Red);
	
	//print r

	color_pixel(112, 150, Red);
	color_pixel(112, 151, Red);
	color_pixel(112, 152, Red);
	color_pixel(112, 153, Red);
	color_pixel(112, 154, Red);

	color_pixel(113, 151, Red);
	color_pixel(114, 150, Red);
	color_pixel(114, 150, Red);
}

void you_won()
{
	//print c

	color_pixel(75, 150, Yellow);
	color_pixel(75, 151, Yellow);
	color_pixel(75, 152, Yellow);
	color_pixel(75, 153, Yellow);
	color_pixel(75, 154, Yellow);

	color_pixel(76, 150, Yellow);
	color_pixel(76, 154, Yellow);

	color_pixel(77, 150, Yellow);
	color_pixel(77, 154, Yellow);

	//print o

	color_pixel(79, 150, Yellow);
	color_pixel(79, 151, Yellow);
	color_pixel(79, 152, Yellow);
	color_pixel(79, 153, Yellow);
	color_pixel(79, 154, Yellow);

	color_pixel(80, 150, Yellow);
	color_pixel(80, 154, Yellow);

	color_pixel(81, 150, Yellow);
	color_pixel(81, 154, Yellow);

	color_pixel(82, 150, Yellow);
	color_pixel(82, 151, Yellow);
	color_pixel(82, 152, Yellow);
	color_pixel(82, 153, Yellow);
	color_pixel(82, 154, Yellow);

	//print n

	color_pixel(84, 151, Yellow);
	color_pixel(84, 152, Yellow);
	color_pixel(84, 153, Yellow);
	color_pixel(84, 154, Yellow);

	color_pixel(85, 150, Yellow);
	color_pixel(86, 150, Yellow);	

	color_pixel(87, 150, Yellow);
	color_pixel(87, 151, Yellow);
	color_pixel(87, 152, Yellow);
	color_pixel(87, 153, Yellow);
	color_pixel(87, 154, Yellow);

	//print g

	color_pixel(89, 150, Yellow);
	color_pixel(89, 151, Yellow);
	color_pixel(89, 152, Yellow);
	color_pixel(89, 154, Yellow);

	color_pixel(90, 150, Yellow);
	color_pixel(90, 152, Yellow);
	color_pixel(90, 154, Yellow);

	color_pixel(91, 150, Yellow);
	color_pixel(91, 151, Yellow);
	color_pixel(91, 152, Yellow);
	color_pixel(91, 153, Yellow);
	color_pixel(91, 154, Yellow);


	//print r

	color_pixel(93, 150, Yellow);
	color_pixel(93, 151, Yellow);
	color_pixel(93, 152, Yellow);
	color_pixel(93, 153, Yellow);
	color_pixel(93, 154, Yellow);

	color_pixel(94, 151, Yellow);
	color_pixel(95, 150, Yellow);
	color_pixel(96, 150, Yellow);

	//print a

	color_pixel(98, 150, Yellow);
	color_pixel(98, 151, Yellow);
	color_pixel(98, 152, Yellow);
	color_pixel(98, 153, Yellow);

	color_pixel(99, 150, Yellow);
	color_pixel(99, 153, Yellow);

	color_pixel(100, 150, Yellow);
	color_pixel(100, 151, Yellow);
	color_pixel(100, 152, Yellow);
	color_pixel(100, 153, Yellow);
	color_pixel(100, 154, Yellow);

	//print t

	color_pixel(102, 150, Yellow);

	color_pixel(103, 149, Yellow);
	color_pixel(103, 150, Yellow);
	color_pixel(103, 151, Yellow);
	color_pixel(103, 152, Yellow);
	color_pixel(103, 153, Yellow);
	color_pixel(103, 154, Yellow);

	color_pixel(104, 150, Yellow);
	color_pixel(104, 154, Yellow);
	
	//print u

	color_pixel(106, 150, Yellow);
	color_pixel(106, 151, Yellow);
	color_pixel(106, 152, Yellow);
	color_pixel(106, 153, Yellow);

	color_pixel(107, 154, Yellow);
	color_pixel(108, 154, Yellow);
	color_pixel(110, 154, Yellow);

	color_pixel(109, 150, Yellow);
	color_pixel(109, 151, Yellow);
	color_pixel(109, 152, Yellow);
	color_pixel(109, 153, Yellow);

	//print l

	color_pixel(112, 150, Yellow);
	color_pixel(112, 151, Yellow);
	color_pixel(112, 152, Yellow);
	color_pixel(112, 153, Yellow);
	color_pixel(112, 154, Yellow);

	color_pixel(113, 154, Yellow);

	//print a

	color_pixel(115, 150, Yellow);
	color_pixel(115, 151, Yellow);
	color_pixel(115, 152, Yellow);
	color_pixel(115, 153, Yellow);

	color_pixel(116, 150, Yellow);
	color_pixel(116, 153, Yellow);

	color_pixel(117, 150, Yellow);
	color_pixel(117, 151, Yellow);
	color_pixel(117, 152, Yellow);
	color_pixel(117, 153, Yellow);
	color_pixel(117, 154, Yellow);

	//print t

	color_pixel(119, 150, Yellow);

	color_pixel(120, 149, Yellow);
	color_pixel(120, 150, Yellow);
	color_pixel(120, 151, Yellow);
	color_pixel(120, 152, Yellow);
	color_pixel(120, 153, Yellow);
	color_pixel(120, 154, Yellow);

	color_pixel(121, 150, Yellow);
	color_pixel(121, 154, Yellow);

	//print i

	color_pixel(123, 148, Yellow);
	color_pixel(123, 150, Yellow);
	color_pixel(123, 151, Yellow);
	color_pixel(123, 152, Yellow);
	color_pixel(123, 153, Yellow);
	color_pixel(123, 154, Yellow);

	//print o

	color_pixel(125, 150, Yellow);
	color_pixel(125, 151, Yellow);
	color_pixel(125, 152, Yellow);
	color_pixel(125, 153, Yellow);
	color_pixel(125, 154, Yellow);

	color_pixel(126, 150, Yellow);
	color_pixel(126, 154, Yellow);

	color_pixel(127, 150, Yellow);
	color_pixel(127, 154, Yellow);

	color_pixel(128, 150, Yellow);
	color_pixel(128, 151, Yellow);
	color_pixel(128, 152, Yellow);
	color_pixel(128, 153, Yellow);
	color_pixel(128, 154, Yellow);

	//print n

	color_pixel(130, 151, Yellow);
	color_pixel(130, 152, Yellow);
	color_pixel(130, 153, Yellow);
	color_pixel(130, 154, Yellow);

	color_pixel(131, 150, Yellow);
	color_pixel(132, 150, Yellow);	

	color_pixel(133, 150, Yellow);
	color_pixel(133, 151, Yellow);
	color_pixel(133, 152, Yellow);
	color_pixel(133, 153, Yellow);
	color_pixel(133, 154, Yellow);

	//print s

	color_pixel(135, 150, Yellow);
	color_pixel(135, 151, Yellow);
	color_pixel(135, 152, Yellow);
	color_pixel(135, 154, Yellow);

	color_pixel(136, 150, Yellow);
	color_pixel(136, 152, Yellow);
	color_pixel(136, 154, Yellow);	

	color_pixel(137, 150, Yellow);
	color_pixel(137, 152, Yellow);
	color_pixel(137, 153, Yellow);
	color_pixel(137, 154, Yellow);

	//print !

	color_pixel(140, 150, Yellow);
	color_pixel(140, 151, Yellow);
	color_pixel(140, 152, Yellow);
	color_pixel(140, 154, Yellow);

	//print !

	color_pixel(142, 150, Yellow);
	color_pixel(142, 151, Yellow);
	color_pixel(142, 152, Yellow);
	color_pixel(142, 154, Yellow);

	//print !

	color_pixel(144, 150, Yellow);
	color_pixel(144, 151, Yellow);
	color_pixel(144, 152, Yellow);
	color_pixel(144, 154, Yellow);
}