/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* 
 * bitAnd - x&y using only ~ and | 
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 *   Max ops: 8
 *   Rating: 1
 */
int bitAnd(int x, int y) {
  return ~(~x|~y);
}
/* 
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int getByte(int x, int n) {
  return (x>>(n<<3))&0xFF;
}
/* 
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3 
 */
int logicalShift(int x, int n) {
  return (x>>n)&~(((0X1<<31)>>n)<<1);
}
/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4 
 */
int bitCount(int x) {
  /* Generate 0x55555555 */
  int mask = 0x55;
  mask += mask<<8;
  mask += mask<<16;
  x = ((x>>1)&mask) + (x&mask);
  /* Generate 0x33333333 */
  mask = 0x33;
  mask += mask<<8;
  mask += mask<<16;
  x = ((x>>2)&mask) + (x&mask);
  /* Generate 0x0F0F0F0F */
  mask = 0x0F;
  mask += mask<<8;
  mask += mask<<16;
  x = ((x>>4)&mask) + (x&mask);
  /* Generate 0xFFFFFFFF */
  mask = 0xFF;
  mask += mask<<16;
  x = ((x>>8)&mask) + (x&mask);
  x += x>>16;
  return x&0xFF;
}
/* 
 * bang - Compute !x without using !
 *   Examples: bang(3) = 0, bang(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int bang(int x) {  
  x = ~x;
  x = (x>>16)&x;
  x = (x>>8)&x;
  x = (x>>4)&x;
  x = (x>>2)&x;
  x = (x>>1)&x;
  return x&0x1;
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 0x1<<31;
}
/* 
 * fitsBits - return 1 if x can be represented as an 
 *  n-bit, two's complement integer.
 *   1 <= n <= 32
 *   Examples: fitsBits(5,3) = 0, fitsBits(-4,3) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int fitsBits(int x, int n) {
  int shift_right = 0x20 + ~n + 0x1;
  int x2 = (x<<shift_right)>>shift_right;
  return !(x^x2);
}
/* 
 * divpwr2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: divpwr2(15,1) = 7, divpwr2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int divpwr2(int x, int n) {
	int bias = (x>>31)&~(((0x1<<31)>>31)<<n);
  	return (x+bias)>>n;
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x+0x1;
}
/* 
 * isPositive - return 1 if x > 0, return 0 otherwise 
 *   Example: isPositive(-1) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 3
 */
int isPositive(int x) {
  return !((x>>31)&0x1)&!!x ;
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  int sgnx = (x>>31)&0x1;
  int sgny = (y>>31)&0x1;
  int d = x + (~y+0x1);
  int sgnd = (d>>31)&0x1;
  return !(x^y) | (sgnx&!sgny) | (!(sgnx^sgny)&sgnd);
}
/*
 * ilog2 - return floor(log base 2 of x), where x > 0
 *   Example: ilog2(16) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 90
 *   Rating: 4
 */
int ilog2(int x) {
  /* Pass the most significant to low bits */
  int mask = 0;
  x |= x>>1;
  x |= x>>2;
  x |= x>>4;
  x |= x>>8;
  x |= x>>16;
  /* Generate 0x55555555 */
  mask = 0x55;
  mask += mask<<8;
  mask += mask<<16;
  x = ((x>>1)&mask) + (x&mask);
  /* Generate 0x33333333 */
  mask = 0x33;
  mask += mask<<8;
  mask += mask<<16;
  x = ((x>>2)&mask) + (x&mask);
  /* Generate 0x0F0F0F0F */
  mask = 0x0F;
  mask += mask<<8;
  mask += mask<<16;
  x = ((x>>4)&mask) + (x&mask);
  /* Generate 0xFFFFFFFF */
  mask = 0xFF;
  mask += mask<<16;
  x = ((x>>8)&mask) + (x&mask);
  x += x>>16;
  return (x&0xFF)+(~0x1+0x1);
}
/* 
 * float_neg - Return bit-level equivalent of expression -f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_neg(unsigned uf) {
	if (uf<<1 > 0xFF000000) 
		return uf;
	return uf + 0x80000000;
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
	// Variable declares
	int f, mask, abs_var, rounded_abs_var, temp, fraction, log2_var, dismissed_bits, last_bit, round_up;

	// If x == 0
	if (!x)
		return 0;
	// If x == tmin
	if (x == 0x80000000)
		return 0xcf000000;

	// If x != 0 && x != tmin
	// Get abs
	if (x > 0) {
		f = 0x3F800000;
		abs_var = x;
	} else {
		f = 0xBF800000;
		abs_var = -x;
	}
	if (abs_var >= 0x7FFFFFC0)
		return 0xcf000000;
	// Get exponent
	mask = 0x40000000;
	log2_var = 30;
	while (!(abs_var & mask)) {
		log2_var--;
		mask >>= 1;
	}
	f += log2_var << 23;
	// Get fraction
	dismissed_bits = log2_var - 23;
	if (dismissed_bits > 0) { 
		// Round
		mask = ~(0xFFFFFFFF << dismissed_bits);
		temp = abs_var & mask;
		rounded_abs_var = abs_var >>= dismissed_bits;
		last_bit = 0x1<<(dismissed_bits-1);
		round_up = 0;
		if (temp > last_bit)
			round_up = 1;
		else if (temp == last_bit)
			if (abs_var & 0x1)
				round_up = 1;
		rounded_abs_var += round_up;
		if (rounded_abs_var / abs_var > 1)
			f += 0x800000;
	} else 
		rounded_abs_var = abs_var <<= -dismissed_bits;
	return f | (rounded_abs_var & 0x7FFFFF);
}
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
  unsigned fraction = uf & 0x7FFFFF;
  unsigned exponet = uf & 0x7F800000;
  // Infinity or NaN
  if (exponet == 0x7F800000) 
  	return uf;
  // Denormal
  else if (!exponet) {
  	fraction <<= 1;
  	fraction &= 0x7FFFFF;
  	// Bordery case
  	if (fraction & 0x400000) 
  	  exponet = 0x800000;
  }
  // Normal
  else {
  	// If result is infnity
  	if (exponet + 0x800000 == 0x7F800000)
  		fraction = 0;
  	exponet += 0x800000;
  }
  // Combine
  uf &= 0x80000000;
  uf |= exponet;
  uf |= fraction;
  return uf;
}