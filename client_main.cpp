#include "client.h"

int main() {
    RadioClient client("255.255.255.255", 10000, 11001, 5000);
    client.work();
}
