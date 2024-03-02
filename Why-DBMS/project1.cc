// Copyright 2022 Wook-shin Han
#include "./project1.h"

void q1(int argc, char ** argv) {
  // Query 1. List all the last names (LNAME)
  // of the active customers that live in Toronto.

  // Save customers' info
  vector < Customer > customers;
  // ifstream customer_file("./test/q1/1/customer.txt");
  ifstream customer_file(argv[2]);
  string customer_line;
  if (customer_file.is_open()) {
    getline(customer_file, customer_line);
    getline(customer_file, customer_line);

    while (getline(customer_file, customer_line)) {
      Customer c;
      string uname = customer_line.substr(0, 20);
      string lname = customer_line.substr(42, 20);
      string zone = customer_line.substr(135, 6);
      string active = customer_line.substr(243, 6);
      // cout << uname << lname << zone << active << endl;
      c.uname = uname.erase(uname.find_last_not_of(' ') + 1);
      c.lname = lname.erase(lname.find_last_not_of(' ') + 1);
      c.zone = stoi(zone.erase(zone.find_last_not_of(' ') + 1));
      c.active = stoi(active.erase(active.find_last_not_of(' ') + 1));

      customers.push_back(c);
    }

  } else {
    cout << "cannot open customer.txt" << endl;
  }

  // save zone ids of Toronto
  set < int > Toronto_id;
  // ifstream zonecost_file("./test/q1/1/zonecost.txt");
  ifstream zonecost_file(argv[3]);
  string zone_line;

  if (zonecost_file.is_open()) {
    getline(zonecost_file, zone_line);
    getline(zonecost_file, zone_line);

    while (getline(zonecost_file, zone_line)) {
      string zone_id = zone_line.substr(0, 6);
      string zone_name = zone_line.substr(7, 20);

      if (zone_name.erase(zone_name.find_last_not_of(' ') + 1) == "Toronto")
        Toronto_id.insert(
          stoi(zone_id.erase(zone_id.find_last_not_of(' ') + 1)));
    }

  } else {
    cout << "cannot open zonecost.txt" << endl;
  }

  // if a customer's zone id matches to Toronto's zone id
  // and the customer is active, print lname of the customer
  for (Customer c : customers) {
    if (Toronto_id.find(c.zone) != Toronto_id.end() && c.active == 1)
      cout << c.lname << endl;
  }

  return;
}

void q2(int argc, char ** argv) {
  // Query 2. Output the BARCODE and the PRODDESC for each product
  // that has been purchased by at least two customers.
  map < int, Product > products;
  ifstream products_file(argv[3]);
  string product_line;
  if (products_file.is_open()) {
    getline(products_file, product_line);
    getline(products_file, product_line);
    while (getline(products_file, product_line)) {
      Product p;
      string barcode = product_line.substr(0, 20);
      int barcode_int = stoi(barcode.erase(barcode.find_last_not_of(' ') + 1));
      string name = product_line.substr(32, 50);
      p.barcode = barcode_int;
      p.name = name.erase(name.find_last_not_of(' ') + 1);
      products.insert({
        barcode_int,
        p
      });
    }
  } else {
    cout << "cannot open products.txt" << endl;
  }
  ifstream lineitem_file(argv[2]);
  string item_line;
  if (lineitem_file.is_open()) {
    getline(lineitem_file, item_line);
    getline(lineitem_file, item_line);
    while (getline(lineitem_file, item_line)) {
      string uname = item_line.substr(0, 20);
      uname = uname.erase(uname.find_last_not_of(' ') + 1);
      string barcode = item_line.substr(41, 20);
      int barcode_int = stoi(barcode.erase(barcode.find_last_not_of(' ') + 1));
      if (products.find(barcode_int) != products.end()) {
        products.find(barcode_int) -> second.customer_set.insert(uname);
      }
    }
  } else {
    cout << "cannot open products.txt" << endl;
  }
  for (auto p : products) {
    if (p.second.customer_set.size() >= 2) {
      cout << p.second.barcode << "                 " << p.second.name << endl;
    }
  }
  return;
}

int main(int argc, char ** argv) {
  string query = argv[1];

  if (query == "q1")
    q1(argc, argv);
  else if (query == "q2")
    q2(argc, argv);

  return 0;
}
