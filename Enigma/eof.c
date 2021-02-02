//
// C emulator for an Enigma Machine with 3 rotors
// Hunter Adams (vha3@cornell.edu)
//
// gcc eof.c -o eof
// 
#include <stdio.h>
#include <string.h>
// EXAMPLE SETTINGS FROM DIAGRAM FOUND ONLINE
// stecker: EBFJACUIHDLKMTOPRQXNGVZSYW - Arbitrary (though must be reciprocal)
// rotor 1: EKMFLGDQVZNTOWYHXUSPAIBRCJ - ROTOR I from online
// rotor 2: AJDKSIRUXBLHWTMCQGZNPYFVOE - ROTOR II from online
// rotor 3: BDFHJLCPRTXVZNYEIWGAKMUSQO - ROTOR III from online
// umkehrw: YRUHQSLDPXNGOKMIEBFZCWVJAT - Non-rotatable Umkehrwalze B from online
char STECKER [27]  = {4,0,3,6,22,23,14,1,25,20,1,25,0,6,0,0,1,25,5,20,12,0,3,21,0,23,0};
char ROTOR1  [27]  = {4,9,10,2,7,1,23,9,13,16,3,8,2,9,10,18,7,3,0,22,6,13,5,20,4,10,0};
char ROTOR2  [27]  = {0,8,1,7,14,3,11,13,15,18,1,22,10,6,24,13,0,15,7,20,21,3,9,24,16,5,0};
char ROTOR3  [27]  = {1,2,3,4,5,6,22,8,9,10,13,10,13,0,10,15,18,5,14,7,16,17,24,21,18,15,0};
char UMKEHR  [27]  = {24,16,18,4,12,13,5,22,7,14,3,21,2,23,24,19,14,10,13,6,8,1,25,12,2,20,0};
// variables to hold rotor positions
volatile int count1, count2, count3 ;
// Maximum size of the message that can be encoded (in character)
#define MAXINPUT 60

// sets rotor positions
void setRotor() {
	char value [5] ;
	printf("\nInput rotor positions (e.g. RNF): ") ;
	scanf("%s", value) ;
	count3=(value[0] - 'A');
	count2=(value[1] - 'A');
	count1=(value[2] - 'A');
}

// allows custom configuration of stecker, rotors, or umkehrwalze
void setElement(int classifier) {
	char peg = 'A' ;
	char c[27] ;
	char temp [27] ;
	int i ;

	switch (classifier)
	{
		case 0: 
			printf("\nInput stecker table: \n") ;
			break ;
		case 1: 
			printf("\nInput rotor 1 table: \n") ;
			break;
		case 2: 
			printf("\nInput rotor 2 table: \n") ;
			break;
		case 3: 
			printf("\nInput rotor 3 table: \n") ;
			break;
		case 4: 
			printf("\nInput umkehrwalze table: \n") ;
			break;
		default: printf("Invalid") ;
	}
	scanf("%s", c) ;

	while (peg <= 'Z') {
		int value = c[peg - 'A'] - peg ;
		if (value<0) value += ('Z' - 'A' + 1) ;

		switch (classifier)
		{
			case 0: 
				STECKER[peg-'A'] = value; 
				break;
			case 1: 
				ROTOR1[peg-'A'] = value; 
				break;
			case 2: 
				ROTOR2[peg-'A'] = value; 
				break;
			case 3: 
				ROTOR3[peg-'A'] = value; 
				break;
			case 4: 
				UMKEHR[peg-'A'] = value; 
				break;
			default: 
				printf("Invalid"); break; 
		}
		peg += 1 ;
	}

	switch (classifier)
		{
			case 0:
				printf("Stecker settings: \n") ;
				for (i=0; i<27; i++) {
					printf("%d ", STECKER[i]) ;
				} 
				break;
			case 1: 
				printf("Rotor 1 settings: \n") ;
				for (i=0; i<27; i++) {
					printf("%d ", ROTOR1[i]) ;
				} 
				break;
			case 2: 
				printf("Rotor 2 settings: \n") ;
				for (i=0; i<27; i++) {
					printf("%d ", ROTOR2[i]) ;
				} 
				break;
			case 3: 
				printf("Rotor 3 settings: \n") ;
				for (i=0; i<27; i++) {
					printf("%d ", ROTOR3[i]) ;
				} 
				break;
			case 4: 
				printf("Umkehrwalze settings: \n") ;
				for (i=0; i<27; i++) {
					printf("%d ", UMKEHR[i]) ;
				} 
				break;
			default: 
				printf("Invalid"); break; 
		}
}

char forwardElement(char Element [27], char input, int countval) {
	// Get the index
	int index = input - 'A' + countval;
	// Make sure the index wraps at 25
	if (index>25) index = (index%26);
	// Update the character value with value at that index
	char output = (input + Element[index]);
	// Range check for values above Z, wrap to A
	if (output>90) output=((output%('Z'+1))+'A');
	// Return value
	return output ;
}


char reverseElement(char Element [27], char output, int countval) {
	// Start at char value 1 below A, outval 0, index 0
	char testval = 'A'-1 ;
	char outval = '0' ;
	int index = 0 ;
	// While the output value does not equal the desired output value
	while (outval != output) {
		// Increment the test character
		testval += 1 ;
		// Get the index of the character
		index = testval - 'A' + countval;
		// Make sure the index wraps at 25
		if (index>25) index = (index%26);
		// Update the test output value with the value at that index
		outval = (testval + Element[index]) ;
		// Range check for values above Z, wrap to A
		if (outval>90) outval=((outval%('Z'+1)) + 'A') ;
	}
	// Return the character that yielded desired output
	return testval ;
}


int main() {
	char c;
	char user_in[MAXINPUT] ;
	char user_out[MAXINPUT] ;
	int i ;
	printf("\n\nAt any time, press 'r' to set rotor configurations and 't' for a total reconfig.\n\n");
	
	setRotor() ;
	// Wait for keyboard entry
	printf("\nInput value (all caps no spaces): ");
	while(1) {
		// Get input
		scanf("%s", user_in) ;
		memset(user_out, 0, MAXINPUT) ;
		int j = 0 ;
		// Grab first character
		c = '0' ;
		while (j<strlen(user_in)) {	
			c = user_in[j] ;	
			if (c=='\r') break ;
			else if (c=='r') setRotor() ;
			else if (c=='t') {
				setElement(0);
				setElement(1);
				setElement(2);
				setElement(3);
				setElement(4);
				setRotor() ;
			}
			else {
				// COMMENT-IN THE PRINT STATEMENTS BELOW TO SEE
				// THE VALUE THAT IS ATTAINED IN EACH STEP OF THE
				// ENCODING CIRCUIT
				//
				// printf("\nInput value: %c", c);
				// Forward stecker
				char value2 = forwardElement(STECKER, c, 0) ;
				// printf("\nStecked value: "); putchar(value2) ;
				// Forward rotor 1
				char value3 = forwardElement(ROTOR1, value2, count1) ;
				// printf("\nRotorized value 1: "); putchar(value3) ;
				// Forward rotor 2
				char value4 = forwardElement(ROTOR2, value3, count2) ;
				// printf("\nRotorized value 2: "); putchar(value4) ;
				// Forward rotor 3
				char value5 = forwardElement(ROTOR3, value4, count3) ;
				// printf("\nRotorized value 3: "); putchar(value5) ;
				// Umkehrwalze
				char value6 = forwardElement(UMKEHR, value5, 0) ;
				// printf("\nUmkehrwalze value: "); putchar(value6) ;

				// print the rotor count1
				// printf("\nRotor count1: %d", count1) ;
				// print the rotor count2
				// printf("\nRotor count2: %d", count2) ;
				// print the rotor count3
				// printf("\nRotor count3: %d", count3) ;

				// Reverse Rotor 3
				char value7 = reverseElement(ROTOR3, value6, count3) ;
				// printf("\nRev. rotor value 3: "); putchar(value7) ;
				// Reverse Rotor 2
				char value8 = reverseElement(ROTOR2, value7, count2) ;
				// printf("\nRev. rotor value 2: "); putchar(value8) ;
				// Reverse Rotor 1
				char value9 = reverseElement(ROTOR1, value8, count1) ;
				// printf("\nRev. rotor value 1: "); putchar(value9) ;
				// Reverse stecker
				char value10 = reverseElement(STECKER, value9, 0) ;
				// printf("\nStecker Output: "); putchar(value10) ;

				user_out[j] = value10 ;

				// Iterate the rotor counters
				count1 = ((count1+1) % 26) ;
				if (count1==0) count2 = ((count2+1)%26) ;
				if ((count1==0)&&(count2==0)) count3 = ((count3+1)%26) ;
			}
			j += 1 ;
		}

		printf("\n\n%s\n\n", user_out) ;

		// Get a new input value
		printf("\n\nInput value (all caps no spaces): ");
	}
}