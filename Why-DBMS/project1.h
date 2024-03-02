// Copyright 2022 Wook-shin Han

#ifndef WHY_DBMS_PROJECT1_H_
#define WHY_DBMS_PROJECT1_H_

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

class Customer{
 public:
    string uname;
    string lname;
    int zone;
    int active;
};

class Product{
 public:
    string name;
    int barcode;
    set<string> customer_set;
};

#endif  // WHY_DBMS_PROJECT1_H_
