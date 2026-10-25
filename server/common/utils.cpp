#include <cmath>

int getRandomNumberInRange(int min, int max){
    int randomNum = min + rand() % (max - min);
    return randomNum; 
}