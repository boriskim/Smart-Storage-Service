//Program for Automated Warehouse Storage Machine
//Also doubles as an "Arcade Claw Machine" because of its scaled down size
#include "JoystickDriver.c"
//Defines the motor associated with each movement as constants 
#define MOTORX motorA
#define MOTORY motorB
#define MOTORZ motorC
#define motorClaw motorD
//Defines the button used on the controller to grab with the claw
#define ENTER_BUTTON Btn1
//Defines the hardware ranges of the controller and motor
#define JOYSTICK_NULLSPACE 10
#define JOYSTICK_MAX 128
#define MOTOR_MAX 100
//Defines the physical length of the claw
#define CLAWLENGTH 4
//Defines the encoder conversion factor for the spool
const float CLAWENCODER = 360 / (1.8 * PI * 2);
//Defines the time limit in seconds
const int TIMELIMIT = 20;

enum Direction
{
	//Used to define the different directions the claw can move along the 3 axes
	DIR_RIGHT,
	DIR_LEFT,
	DIR_FORWARD,
	DIR_BACKWARD,
	DIR_UP,
	DIR_DOWN
};

void movePosition(Direction moveDir, int power)
{
	//Sets the appropriate motor to a valid motor power so that the claw moves in the direction given in the parameter
	if (power < 0 || power > 100)
		return;
	switch(moveDir)
	{
	case DIR_RIGHT:
		motor[MOTORY] = -power;
		break;
	case DIR_LEFT:
		motor[MOTORY] = power;
		break;
	case DIR_FORWARD:
		motor[MOTORX] = -power;
		break;
	case DIR_BACKWARD:
		motor[MOTORX] = power;
		break;
	case DIR_UP:
		motor[MOTORZ] = -power;
		break;
	case DIR_DOWN:
		motor[MOTORZ] = power;
		break;
	}
}

void stopMovement(Direction stopDir)
{
	//Stops all movement in a given axis
	switch(stopDir)
	{
	case DIR_RIGHT:
	case DIR_LEFT:
		motor[MOTORY] = 0;
		break;
	case DIR_FORWARD:
	case DIR_BACKWARD:
		motor[MOTORX] = 0;
		break;
	case DIR_UP:
	case DIR_DOWN:
		motor[MOTORZ] = 0;
		break;
	}
}

void resetPosition()
{
	//Moves the claw back to the home position over the drop box using the touch sensors
	movePosition(DIR_LEFT, 50);

	while(SensorValue[S1] == 0)
	{}

	stopMovement(DIR_LEFT);

	movePosition(DIR_FORWARD, 100);

	while(SensorValue[S2] == 0)
	{}

	stopMovement(DIR_FORWARD);
}

void grip(bool command)
{
	//Closes or opens the claw depending on which parameter it is called with
	if(command)
	{
		motor[motorClaw] = -40;
		wait1Msec(3000);
		motor[motorClaw] = 0;
	}
	else
	{
		motor[motorClaw] = 40;
		wait1Msec(3000);
		motor[motorClaw] = 0;
	}
}

int getCredit()
{
	//Checks if and what type of ID has been inserted into the ID card slot using the colour sensor
	if(SensorValue[S4] == (int)colorBlue)
		return 2;
	else if(SensorValue[S4] == (int)colorGreen)
		return 1;
	else
		return 0;
}

task displayTimer()
{
	//Independent task used to keep the timer display updated while the rest of the program happens
	//Uses multitasking to run in parallel with the main task
	clearTimer(timer1);
	int lastTime = 0;
	while(time1[timer1]/1000 < TIMELIMIT)
	{
		if(time1[timer1] - lastTime > 1000)
		{
			eraseDisplay();
			displayCenteredBigTextLine(5, "Time Left:");
			displayCenteredBigTextLine(7, "%d", TIMELIMIT - time1[timer1]/1000);
			lastTime = time1[timer1];
		}
		//Allows for multitasking
		abortTimeslice();
	}
}

bool checkTimer()
{
	//Checks to see if timer has reached the time limit
	if(time1[timer1] < TIMELIMIT * 1000)
		return true;
	else
		return false;
}

void startTimer()
{
	//Used for multitasking, starts the displayTimer task
	startTask(displayTimer);
}

void endTimer()
{
	//Used for multitasking, ends the displayTimer task and resets the display
	stopTask(displayTimer);
	eraseDisplay();

}

int convertRawInputToPower(int rawInput)
{
	//Converts the values returned by the joysticks (-128 to 128) into corresponding absolute motor power values (0 to 100)
	const float conversionFactor = 100.0 / 128;
	//displayCenteredBigTextLine(3, "%f", rawInput * conversionFactor); (Debug)
	return abs(rawInput) * conversionFactor;
}

void JoyXYMove()
{
	//Uses the JoystickDriver.c code in the RobotC library to get user input from an DirectInput bluetooth controller connected to the laptop which sends the data to the EV3 brick over a USB debug link
	//Constantly checks positions of joysticks and updates motor power appropriately until timer ends or user presses the ENTER_BUTTON (defined as the A button on controller)
	//NULLSPACE is the area around the centre of the joystick that is not counted as user input, claw is stopped until the user moves the joystick outside this area
	getJoystickSettings(joystick);
	while(checkTimer() && joy1Btn(ENTER_BUTTON) != 1)
	{
		getJoystickSettings(joystick);
		if(joystick.joy1_x1 > JOYSTICK_NULLSPACE)
		{
			movePosition(DIR_RIGHT, convertRawInputToPower(joystick.joy1_x1));
		}
		else if(joystick.joy1_x1 < -JOYSTICK_NULLSPACE)
		{
			movePosition(DIR_LEFT, convertRawInputToPower(joystick.joy1_x1));
		}
		else
		{
			stopMovement(DIR_LEFT);
		}
		if(joystick.joy1_y1 > JOYSTICK_NULLSPACE)
		{
			movePosition(DIR_BACKWARD, convertRawInputToPower(joystick.joy1_y1));
		}
		else if(joystick.joy1_y1 < -JOYSTICK_NULLSPACE)
		{
			movePosition(DIR_FORWARD, convertRawInputToPower(joystick.joy1_y1));
		}
		else
		{
			stopMovement(DIR_FORWARD);
		}
		//Used for multitasking with the displayTimer task
		abortTimeslice();
		//Debug code used to manually control functions of the claw during testing, disabled for final program
		/*
		if(joystick.joy1_TopHat == 7 || joystick.joy1_TopHat == 0 || joystick.joy1_TopHat == 1)
			movePosition(DIR_UP, 20);
		else if(joystick.joy1_TopHat == 3 || joystick.joy1_TopHat == 4 || joystick.joy1_TopHat == 5)
			movePosition(DIR_DOWN, 20);
		else
			stopMovement(DIR_UP);
		if(joystick.joy1_TopHat == 1 || joystick.joy1_TopHat == 2 || joystick.joy1_TopHat == 3)
			grip(false);
		else if(joystick.joy1_TopHat == 5 || joystick.joy1_TopHat == 6 || joystick.joy1_TopHat == 7)
			grip(true);
		else
			;
		if(joy1Btn(Btn5))
			resetPosition();
			*/
	}
	//Stops all movement of the claw
	stopMovement(DIR_RIGHT);
	stopMovement(DIR_FORWARD);
}

//Depreciated code for control using EV3 Buttons
/*void XYMove()
{
while(checkTimer() && getButtonPress(buttonEnter) == 0)
{
if(getButtonPress(buttonLeft) == 1)
{
movePosition(DIR_LEFT, 20);

while(getButtonPress(buttonLeft) == 1)
{}

stopMovement(DIR_LEFT);
}
else if(getButtonPress(buttonRight) == 1)
{
movePosition(DIR_RIGHT, 20);

while(getButtonPress(buttonRight) == 1)
{}

stopMovement(DIR_RIGHT);
}
else if(getButtonPress(buttonUp) == 1)
{
movePosition(DIR_FORWARD, 20);

while(getButtonPress(buttonUp))
{}

stopMovement(DIR_FORWARD);
}
else if(getButtonPress(buttonDown) == 1)
{
movePosition(DIR_BACKWARD, 20);

while(getButtonPress(buttonDown))
{}

stopMovement(DIR_BACKWARD);
}
}
}
*/

int getUltrasonicConsistent()
{
	//Takes 5 separate ultrasonic readings, then compares each to the initial reading to see if at least 2 are consistent with the initial reading
	//If so, it uses the initial reading
	//Otherwise, it redoes the reading procedure until it receives consistent values
	//Takes into account the high uncertainty of the ultrasonic sensor at any given instant
	const int ultrasonicTolerance = 2; //Readings are considered equivalent if they are within 2cm of each other
	int ultrasonicReadings[5] = {0,0,0,0,0};
	int numConsistentReadings = 0;
	do
	{
		numConsistentReadings = 0;
		for(int readingNum = 0; readingNum < 5; readingNum++)
		{
			ultrasonicReadings[readingNum] = SensorValue[S3];
			wait1Msec(50);
		}
		for(int readingNum = 1; readingNum < 5; readingNum++)
		{
			if(abs(ultrasonicReadings[0] - ultrasonicReadings[readingNum]) < ultrasonicTolerance)
				numConsistentReadings++;
		}
	} while(numConsistentReadings <= 2);

	return ultrasonicReadings[0];
}

void grab()
{
	//Detects the distance from the claw to the nearest surface, then lowers the claw by that distance using the calculated motor encoder limit.
	//Closes the claw once it's lowered, then raises it back up the opposite amount it was lowered
	nMotorEncoder[MOTORZ] = 0;

	int drop_distance = 0;
	drop_distance = getUltrasonicConsistent() - CLAWLENGTH;
	if(drop_distance > 90)
		drop_distance = 90;
	while(abs(nMotorEncoder[MOTORZ]) < drop_distance * CLAWENCODER)
	{
		movePosition(DIR_DOWN, 40);
	}

	stopMovement(DIR_DOWN);

	grip(true);

	movePosition(DIR_UP, 40);

	while(nMotorEncoder[MOTORZ] > 0)
	{}

	stopMovement(DIR_UP);
}

void displayWelcomeMessage()
{
	//Displays the default message for when the machine is waiting for a ticket
	setLEDColor(ledOrangeFlash);
	eraseDisplay();
	displayCenteredBigTextLine(5, "Insert a ticket");
	displayCenteredBigTextLine(7, "to play!");
}

void displayIDInserted()
{
	//Displays a message and audible feedback
	setLEDColor(ledOrangePulse);
	eraseDisplay();
	displayCenteredBigTextLine(5, "Ready to play!");
	displayCenteredBigTextLine(7, "Start moving");
	displayCenteredBigTextLine(9, "to begin game!");
	playSound(soundShortBlip);
}

//The following functions are for the claw machine variation

bool testForWin()
{
	//Tests if the claw has grabbed a prize by sensing if the ultrasonic sensor is reporting maximum values (which means the distance is out of the operating range of the sensor, which means the prize is against the sensor)
	if(getUltrasonicConsistent() > 250)
		return true;
	else
		return false;
}

void displayCreditsRemainingMessage(int credits)
{
	//Displays the message for when there are still credits remaining
	setLEDColor(ledGreen);
	displayCenteredBigTextLine(5, "You have %d", credits);
	displayCenteredBigTextLine(7, "credit(s)");
	displayCenteredBigTextLine(9, "remaining");
}

void displayWinMessage()
{
	//Displays a win message, sets the LEDs to flashing green and plays a happy tone
	setLEDColor(ledGreenFlash);
	eraseDisplay();
	displayCenteredBigTextLine(7, "Congratulations!");
	displayCenteredBigTextLine(9, "You won!!");
	playSound(soundFastUpwardTones);
	wait1Msec(300);
	playSound(soundFastUpwardTones);
	wait1Msec(300);
	playSound(soundFastUpwardTones);
	wait1Msec(300);
}

void displayLoseMessage()
{
	//Displays a lose message, sets the LEDs to flashing red and plays a sad tone
	setLEDColor(ledRedFlash);
	eraseDisplay();
	displayCenteredBigTextLine(5, "Too bad,");
	displayCenteredBigTextLine(7, "you lost!");
	displayCenteredBigTextLine(9, "Try again!");
	playSound(soundDownwardTones);
}

void eraseMessages()
{
	//Resets the message and LED colour on the EV3
	setLEDColor(ledOrange);
	eraseDisplay();
}

task main()
{
	SensorType[S1] = sensorEV3_Touch; //X-direction stopMovement bumper
	SensorType[S2] = sensorEV3_Touch; //Y-direction stopMovement bumper
	SensorType[S3] = sensorEV3_Ultrasonic; //Ultrasonic sensor mounted on claw
	SensorType[S4] = sensorEV3_Color; //ID card slot sensor
	wait1Msec(50);
	SensorMode[S4] = modeEV3Color_Color;
	wait1Msec(50);

	int credit = 0;

	//Resets claw position
	resetPosition();
	displayWelcomeMessage();

	//Enter button on the EV3 is used to exit the program
	while(!getButtonPress(buttonEnter))
	{
		if(credit < 1)
			//Waits for user to insert ID Card
			credit = getCredit();
		else
		{
			displayIDInserted();
			while(credit > 0)
			{
				credit--;
				getJoystickSettings(joystick);
				//Once a ticket is inserted, the program waits for input on the joystick
				while(abs(joystick.joy1_x1) < JOYSTICK_NULLSPACE && abs(joystick.joy1_y1) < JOYSTICK_NULLSPACE)
				{
					getJoystickSettings(joystick);
				}
				eraseMessages();
				//Starts timer, then moves claw according to joystick inputs while timer is running or until user presses the A button on the controller
				startTimer();
				JoyXYMove();
				//When timer ends, displayTimer task is stopped and claw attempts to grab then drop prize in drop box
				endTimer();
				grab();
				bool win = testForWin();
				resetPosition();
				wait1Msec(800);
				grip(false);
				//Displays appropriate message depending on whether the player successfully grabbed a prize
				if(win)
					displayWinMessage();
				else
					displayLoseMessage();
				wait1Msec(5000);
				eraseMessages();
				//Displays the appropriate message depending on whether the player still has credits remaining
				if(credit < 1)
					displayWelcomeMessage();
				else
					displayCreditsRemainingMessage(credit);
			}
		}
	}
}
