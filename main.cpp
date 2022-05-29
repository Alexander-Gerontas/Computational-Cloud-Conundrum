#include "cJSON.c"
#include "cJSON.h"
#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <map>
#include <fstream>

using namespace std;

class Stack
{
private:
    string name;
    list<string> dependencyList;

public:
    Stack(string name, list<string> dependencyList)
    {
        this->name = name;
        this->dependencyList = dependencyList;
    }

    string get_name()
    {
        return name;
    }

    int get_dependency_number()
    {
        return dependencyList.size();
    }

    list<string> get_dependencies()
    {
        return dependencyList;
    }

    // Δέχεται μια λίστα με dependencies και ελέγχει αν υπάρχουν στο stack
    bool containsDependencies(list<string> dependencies)
    {
        for (string stackDependency : dependencyList)
        {
            bool dep_found = false;
            for (string list_dependency : dependencies)
            {
                if (stackDependency == list_dependency)
                {
                    dep_found = true;
                    break;
                }
            }
            if (!dep_found)
                return false;
        }

        return true;
    }
};

class Environment
{
private:
    string name;
    list<Stack *> dependencyList;

public:
    Environment(string name, list<Stack *> myList)
    {
        this->name = name;
        this->dependencyList = myList;
    }

    // Προσθήκη Stack στο περιβάλλον
    void add_dependency(Stack *dependency)
    {
        if (!containsDependency(dependency))
            dependencyList.push_back(dependency);
        else
            cout << "dependency already exists in environment" << endl;
    }

    // Τύπωση των Stack που περιέχει το περιβάλλον
    void print_dependencyList()
    {
        int i = 0;
        for (Stack *dependency : dependencyList)
            cout << i++ << ". " << dependency->get_name() << endl;
    }

    string get_environment_name()
    {
        return name;
    }

    int get_stack_num()
    {
        return dependencyList.size();
    }

    list<Stack *> get_dependencyList()
    {
        return dependencyList;
    }

    // Ελέγχει αν το Stack υπάρχει στο περιβάλλον
    bool containsDependency(Stack *dependency)
    {
        for (Stack *environDependency : dependencyList)
            if (environDependency->get_name() == dependency->get_name())
                return true;

        return false;
    }

    // Αποθηκεύει το περιβάλλον σε αρχείο
    void saveJson(string filename)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON *env_array = cJSON_CreateArray();
        cJSON_AddItemToObject(root, "environment", env_array);

        cJSON *env_name = cJSON_CreateObject();
        cJSON_AddItemToArray(env_array, env_name);

        // Προσθήκη του ονόματος του περιβάλλοντος στο json object
        cJSON_AddItemToObject(env_name, "name", cJSON_CreateString(get_environment_name().c_str()));

        cJSON *stacks_array = cJSON_CreateArray();
        cJSON_AddItemToObject(env_name, "stacks", stacks_array);

        // Προσθήκη όλων των stack στο json array
        for (Stack *envDependency : dependencyList)
        {
            cJSON_AddItemToArray(stacks_array, cJSON_CreateString(envDependency->get_name().c_str()));
        }

        string out = cJSON_Print(root);

        // Αφαίρεση κενού και newline
        replace(out.begin(), out.end(), '\n', ' ');
        out.erase(remove_if(out.begin(), out.end(), ::isspace), out.end());

        // Αποθήκευση αρχείου
        ofstream outfile;
        outfile.open(filename);
        outfile << out << endl;
        outfile.close();
    }
};

class Stacks
{
private:
    map<string, Stack *> stack_map;

public:
    Stacks() {}

    // Προσθήκη νέου stack στο λεξικό
    void add_new_stack(string stack_name, Stack *stack)
    {
        stack_map[stack_name] = stack;
    }

    // Επιστροφή του Stack που αντιστοιχεί στο stack_name
    Stack *get_stack_by_name(string stack_name)
    {
        return stack_map[stack_name];
    }

    // Επιστροφή του Stack που αντιστοιχεί στο index
    Stack *get_stack_by_index(int index)
    {
        auto it = stack_map.begin();
        advance(it, index);
        return it->second;
    }

    // Τύπωση όλων των Stack στο λεξικό
    void print_map_stacks()
    {
        map<string, Stack *>::iterator it;
        int i = 0;

        for (it = stack_map.begin(); it != stack_map.end(); it++)
        {
            cout << i++ << ". ";
            cout << it->second->get_name() << endl;
        }
    }

    // Δημιουργία περιβάλλοντος από αρχείο json
    Environment *create_environment(string line)
    {
        cJSON *elem;
        cJSON *environment_name;
        cJSON *dependencies;
        cJSON *environment;
        cJSON *dependency;
        cJSON *root = cJSON_Parse(line.c_str());

        elem = cJSON_GetArrayItem(root, 0);
        int dep_num = cJSON_GetArraySize(elem);

        environment = cJSON_GetArrayItem(elem, 0);

        environment_name = cJSON_GetObjectItem(environment, "name");

        // Εύρεση του ονόματος του περιβάλλοντος
        string environ_name = environment_name->valuestring;

        dependencies = cJSON_GetObjectItem(environment, "stacks");

        int total_stacks = cJSON_GetArraySize(dependencies);

        list<Stack *> environment_dependencies;

        Stack *stack;

        // Εύρεση των stack στο περιβάλλον και προσθήκη στη λίστα
        for (int j = 0; j < total_stacks; j++)
        {
            dependency = cJSON_GetArrayItem(dependencies, j);

            stack = stack_map[dependency->valuestring];

            environment_dependencies.push_back(stack);
        }

        // Δημιουργία νέου περιβάλλοντος
        return new Environment(environ_name, environment_dependencies);
    }

    // Ελέγχει αν όλα τα dependency υπάρχουν στο περιβάλλον
    bool check_environment_dependencies(Environment *environment, bool print_missing)
    {
        bool dependencies_satisfied = true;
        Stack *dependency;

        // Εύρεση των dependency του περιβάλλοντος
        for (Stack *stack : environment->get_dependencyList())
        {
            for (string dep_name : stack->get_dependencies())
            {
                // Μετατροπή του dependency σε αντικείμενο stack
                dependency = get_stack_by_name(dep_name);

                // Ελέγχει αν το stack υπάρχει στο περιβάλλον
                if (!environment->containsDependency(dependency))
                {
                    dependencies_satisfied = false;
                    if (print_missing)
                        cout << stack->get_name() << " dependency " << dep_name << " missing" << endl;
                }
            }
        }

        if (dependencies_satisfied && print_missing)
            cout << "no dependencies missing" << endl;

        return dependencies_satisfied;
    }

    // Δέχεται σαν όρισμα ένα αντικείμενο Εnvironment και ταξινομεί τα Stacks σε αυτό
    Environment *serialize_dependency_order(Environment *environment)
    {
        list<Stack *> order_list;
        list<string> dep_list;

        list<Stack *> env_stacks = environment->get_dependencyList();
        list<Stack *> dependencies(env_stacks);

        for (int i = 0; i < environment->get_stack_num(); i++)
        {
            // Αντιγραφή της λίστας με τα dependencies
            list<Stack *> tmp_list(dependencies);

            for (Stack *stack : tmp_list)
            {
                // Προσθήκη του stack στην λίστα αν δεν έχει dependencies ή αν ολα τα dependency υπάρχουν στη λίστα
                if (stack->get_dependency_number() == 0 || stack->containsDependencies(dep_list))
                {
                    order_list.push_back(stack);
                    dep_list.push_back(stack->get_name());
                    dependencies.remove(stack);
                    break;
                }
            }
        }

        // Επιστροφή νέου αντικειμένου Environment
        return new Environment(environment->get_environment_name(), order_list);
    }
};

// Φόρτωση των stack από αρχείο json σε αντικείμενο Stacks
Stacks *load_stacks(string filename)
{
    Stacks *stacks = new Stacks();
    Stack *stack_object;

    cJSON *root = cJSON_Parse(filename.c_str());
    cJSON *elem;
    cJSON *json_stack_name;
    cJSON *dependencies;
    cJSON *stacks_json_array;
    cJSON *dependency;

    elem = cJSON_GetArrayItem(root, 0);
    int n1 = cJSON_GetArraySize(elem);

    for (int i = 0; i < n1; i++)
    {
        stacks_json_array = cJSON_GetArrayItem(elem, i);
        json_stack_name = cJSON_GetObjectItem(stacks_json_array, "name");

        // Εύρεση του ονόματος του stack
        string stack_name = json_stack_name->valuestring;

        dependencies = cJSON_GetObjectItem(stacks_json_array, "dependencies");

        int n2 = cJSON_GetArraySize(dependencies);

        list<string> dependency_list;

        // Εύρεση των dependency του stack
        for (int j = 0; j < n2; j++)
        {
            dependency = cJSON_GetArrayItem(dependencies, j);
            dependency_list.push_back(dependency->valuestring);
        }

        // Δημιουργία νέου αντικειμένου stack
        stack_object = new Stack(stack_name, dependency_list);

        // Προσθήκη του stack στην κλάση stacks
        stacks->add_new_stack(stack_name, stack_object);
    }

    return stacks;
}

int main()
{
    ifstream infile("stacks.txt");
    string filename;

    if (!infile)
    {
        cout << "stacks file does not exist." << endl;
        exit(EXIT_FAILURE);
    }

    getline(infile, filename);
    infile.close();

    // Φόρτωση των stack από το αρχείο stacks.txt
    Stacks *stacks = load_stacks(filename);

    int input = -1;
    Environment *environment;

    while (true)
    {
        cout << "0. Exit" << endl;
        cout << "1. load Environment from file" << endl;
        cout << "2. add dependency to Environment" << endl;
        cout << "3. verify Environment dependencies" << endl;
        cout << "4. serialize Environment dependencies" << endl;
        cout << "5. save Environment\n>> ";

        cin >> input;

        if (input == 0)
            exit(EXIT_SUCCESS);

        else if (input == 1)
        {
            string inputfile;

            cout << "enter input filename\n>> ";

            cin >> inputfile;
            infile.open(inputfile);

            if (!infile)
            {
                cout << "Environment file does not exist." << endl;
                exit(EXIT_FAILURE);
            }

            getline(infile, filename);
            infile.close();

            environment = stacks->create_environment(filename);
            cout << "\nenvironment name: " << environment->get_environment_name() << endl;
            cout << "dependencies: " << endl;
            environment->print_dependencyList();
            cout << endl;
        }

        else if (input == 2 && environment)
        {
            int selection = -1;
            stacks->print_map_stacks();

            cout << "select a dependency\n>> ";
            cin >> selection;

            Stack *dep = stacks->get_stack_by_index(selection);
            environment->add_dependency(dep);

            cout << "\ndependencies: " << endl;
            environment->print_dependencyList();
            cout << endl;
        }

        else if (input == 3 && environment)
        {
            stacks->check_environment_dependencies(environment, true);
            cout << endl;
        }

        else if (input == 4 && environment)
        {
            if (stacks->check_environment_dependencies(environment, false))
            {
                environment = stacks->serialize_dependency_order(environment);
                cout << "\ndependencies: " << endl;
                environment->print_dependencyList();
                cout << endl;
            }
            else
                cout << "provide all dependencies first" << endl;
        }

        else if (input == 5 && environment)
        {
            string output_filename;

            cout << "enter output filename\n>>";
            cin >> output_filename;

            environment->saveJson(output_filename);
        }

        else if (!environment)
            cout << "environment not loaded" << endl;
    }

    return 0;
}