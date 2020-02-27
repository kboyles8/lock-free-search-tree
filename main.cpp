#include <iostream>

#include "treap.h"

using namespace std;

int main(void) {
    Treap treap;

    int size = treap.getSize();

    cout << "The treap has " << to_string(size) << " nodes" << endl;
}