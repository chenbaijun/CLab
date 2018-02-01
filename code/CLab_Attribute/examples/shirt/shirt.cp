type
 shirtColor {Black,White,Red,Blue};
 shirtSize  {Small,Medium,Large};
 shirtPrint {MIB,STW}; 
variable
 shirtColor color;
 shirtSize  size;
  shirtPrint print;
rule
 (print==MIB)>>(color==Black); //3种可行解
 (print==STW)>>(size!=Small);  //8种可行解
//共计11种可行解
