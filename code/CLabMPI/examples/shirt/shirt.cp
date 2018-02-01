type
 shirtColor {Black,White,Red,Blue};
 shirtSize  {Small,Medium,Large};
 shirtPrint {MIB,STW}; 
variable
 shirtColor color;
 shirtSize  size;
  shirtPrint print;
rule
 (print==MIB)>>(color==Black);
 (print==STW)>>(size!=Small);  
