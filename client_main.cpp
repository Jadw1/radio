#include "client.h"

int main() {
    RadioClient client("255.255.255.255", 10000, 11000, 5000);
    client.work();
}
