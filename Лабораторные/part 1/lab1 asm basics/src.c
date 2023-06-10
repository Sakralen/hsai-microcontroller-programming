//#include "stm32f10x.h"

//объявление переменных
//объявление переменных
unsigned short usArr1[5] = {2, 3, 4};          //массив элементов ushort
signed   char  scMas2[4] = {1, 2, 3, 4};     //массив элементов char
signed short ssKnv1;
volatile unsigned int uiKnv1;
signed char scKnv1, scKnv2, scKnv3;
unsigned short * puiKnv1;
unsigned int uiMy1, uiMy2;

const int ciKnv4 = 12;

int main(void) {  
	
	char cByte = 'A';
	cByte = cByte + 3;
	
	uiMy1 = 0x3;
	uiMy2 = uiMy1;	
	uiMy1 = 0x7ff;
	uiMy1 = 65536;
	uiMy1 = 0x12345678;

	
	
	scKnv1=100;            //положительное целое со знаком в пределах байта 
	scKnv2=scKnv3=-100;    // цепочка присваиваний отрицательного целого в пределах байта
	uiKnv1=0x134000;		 // положительное целое 3 байта 
	uiKnv1=0x6888;		 // положительное целое в пределах 2 байт
	uiKnv1=-0x1234CC34;	 // отрицательное целое в пределах 4 байт
  
	usArr1[3] = ciKnv4;
	usArr1[4] = 12;
	
	
	puiKnv1 = usArr1; //присваиваем указателю адрес первого элемента массива
	*puiKnv1 = 0x4321; //тому на что ссылается указатель (а ссылается он на 4 байта, т.к. тип указателя ui)
	//присваиваем значение константы 0х4321 - положиетльное целое в пределах 2 байт
	puiKnv1 = (unsigned short *) ((unsigned int)usArr1 + 1); // получаем из адреса uint,
	//увеличиваем uint на 1 и приводим к указателю на ushort
	//теперь puiKnv1 ссылается на 2 ячейки массива usArr1 c номерами 1 и 2 (нумерация от 0)
	*puiKnv1 = 0x1234; //изменяем то на что ссылается указатель

	for (uiKnv1=2;uiKnv1<18;uiKnv1+=3)   {       //  Цикл
		usArr1[uiKnv1]=(uiKnv1>>1)+13;            //  Операция
	}
	return 0;
}
