#include <iostream>
#include "radio.h"

int main() {
    Radio radio("waw02-03.ic.smcdn.pl", "8000");
    radio.play("/t050-1.mp3", true);
}
