/******************************************************************************\

Программа-заготовка для работы "Архитектура ЦВМ. Подпрограммы"

Задание.
Написать собственную функцию Fun( 5 параметров ), выполняющую арифметические действия со всеми параметрами. 
Функция должна использовать локальные переменные.
Функция: возвращает значение/принимает параметр 1/параметр 2/параметр 3/...
указатель в качестве параметра - массив из трёх элементов

№.  возвр. знач/параметры
1.  char       /int    / double / double* / int*   / double
2.  char       /double / double / char*   / int*   / double
3.  int        /char   / char   / float*  / int*   / double
4.  float      /float  / float  / int*    / int*   / double
5.  short      /int*   / char   / short   / int*   / double
6.  short      /short  /short*  / float   / int*   / double
7.  double     /int    / char   / short*  / int*   / double
8.  int        /double/ double  / double* / int*   / double
9.  float      / int   / char   / short*  / int*   / double
10. double     / char  / int*   / double  / int*   / double

- для сборки ARM (без оптимизации - level 0) в режиме отладки
- определить и описать команды, используемые для
  - сохранения контекста (при вызове функции), указать где, каким образом, в каком объёме сохраняется контекст
	- восстановления контекста
	- передачи параметров (указать каким образом передаются параметры)
	- возврата значения из функции (указать каким образом передаётся значение)
	- определить где и каким образом сохраняется адрес возврата из функции
- при использовании стека описать каким образом переменные попадают в стек, извлекаются из него (указать используемые команды,
прокомментировать способ формирования адреса переменных, находящихся в стеке)
- подготовить и защитить отчёт

Дополнительное задание (для не защитивших отчёт во время):
Используя MS Visual studio, выполнить аналогичное задание для сборки *86 (без оптимизации)


\***********************************************************************/

#define A1 123	// = 0x7B
#define M1 0x000000F0
//#define S2 76543
//#define A2 6543 // = 0x198F
//#define M2 0xFFFFF0FF			//  Конец блока констант


volatile unsigned int uiMas[20] = { 0, 1, 2, 3 };  //  Массив элементов заданного типа

int Fun(unsigned int a, unsigned int b, unsigned int c, unsigned int d, unsigned int e) {
	int iLoc;
	iLoc = a;
	iLoc *= (b - c + d);
	iLoc = (iLoc - A1) | (M1 & e);
	return iLoc;
}

char MyFun(int iA, double dB, double* pdC, int* piD, double dE) {
	char cLoc = 0x64;
	double dLoc;
	int iLoc;
	dLoc = dB * pdC[0] - pdC[1] + pdC[2] + dE;
	iLoc = iA + piD[0] - piD[1] + piD[2];
	cLoc += dLoc - iLoc;
	return cLoc;
}

int main(void) {  //

	/*int a = 1, b = 2, c = 3, d = 4, e = 5, f = 6, g = 7, h = 8, i = 9, j = 10, k = 11, l = 12;

	a += b += c += d += e += g += f += h += i += k += l += j;

	uiMas[1] = Fun(a, b, c, d, 0xabcd);

	a += b += c += d += e += g += f += h += i += k += l += j;*/
	
	int iA = 13;
	double dB = 16663.5656631;
	double pdC[] = {231231.454, 34234.4543, 45345.54545};
	int piD[] = {10, 11, 12};
	double dE = 1313.1534534;
	
	int iTmp = iA + piD[2];
	double dTmp = dB - pdC[0] -pdC[1];
	iTmp *= dTmp;
	
	MyFun(iA, dB, pdC, piD, dE);

	return 0;
}
