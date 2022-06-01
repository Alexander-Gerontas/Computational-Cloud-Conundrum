#include "cJSON.c"
#include "cJSON.h"
#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <map>
#include <fstream>
#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/topological_sort.hpp"

using namespace std;
using namespace boost;

class Stack
{
private:
    string name;
    list<Stack *> dependencyList;
    int index1;

public:
    Stack(string name, list<Stack *> dependencyList)
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

    list<Stack *> get_dependencies()
    {
        return dependencyList;
    }
};

typedef boost::adjacency_list<listS, vecS, bidirectionalS, Stack *, no_property> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor CustomVertex;
typedef std::vector<CustomVertex> container;

class CustomGraph
{
private:
    map<string, CustomVertex> vertex_map;
    Graph g;
    container c;

public:
    void add_new_vertex(Stack *stack)
    {
        string stack_name = stack->get_name();
        vertex_map[stack_name] = add_vertex(stack, g);
    }

    void add_new_edge(string from, string to)
    {
        CustomVertex v0 = vertex_map[from];
        CustomVertex v1 = vertex_map[to];

        // Έλεγχος αν η ακμή υπάρχει
        if (!edge(v0, v1, g).second)
            add_edge(v0, v1, g);
    }

    // Επιστροφή τoυ αριθμού των ακμών προς ένα κόμβο
    int get_graph_edges(string vertex_name)
    {
        CustomVertex v0 = vertex_map[vertex_name];
        int edges = in_degree(v0, g);
        return edges;
    }

    list<Stack *> topological_sorting()
    {
        // Ταξινόμηση των stack
        topological_sort(g, std::back_inserter(c));

        list<Stack *> stack_list;

        // Προσθήκη των stack από τον γράφο σε λίστα
        for (container::reverse_iterator ii = c.rbegin(); ii != c.rend(); ++ii)
            stack_list.push_back(g[*ii]);

        return stack_list;
    }
};

class Environment
{
private:
    string name;
    list<Stack *> stackList;
    set<Stack *> missing_stackList;
    CustomGraph environment_graph;

public:
    Environment(string name)
    {
        this->name = name;
    }

    // Προσθήκη Stack στο περιβάλλον
    void add_stack(Stack *stack)
    {
        if (!containsStack(stack))
        {
            stackList.push_back(stack);

            // Δημιουγία νέου κόμβου
            environment_graph.add_new_vertex(stack);

            // Προσθήκη ακμών προς τον κόμβο
            for (Stack *tmp_stack : stack->get_dependencies())
            {
                if (containsStack(tmp_stack))
                    environment_graph.add_new_edge(tmp_stack->get_name(), stack->get_name());
                else
                    missing_stackList.insert(stack);
            }
        }
        else
            cout << "dependency already exists in environment" << endl;
    }

    // Τύπωση των Stack που περιέχει το περιβάλλον
    void print_dependencyList()
    {
        int i = 0;
        for (Stack *dependency : stackList)
            cout << i++ << ". " << dependency->get_name() << endl;
    }

    string get_environment_name()
    {
        return name;
    }

    // Ελέγχει αν το Stack υπάρχει στο περιβάλλον
    bool containsStack(Stack *dependency)
    {
        for (Stack *environDependency : stackList)
            if (environDependency->get_name() == dependency->get_name())
                return true;

        return false;
    }

    void fix_graph()
    {
        if (missing_stackList.size() == 0)
            return;

        set<Stack *> missing_dependencyList2(missing_stackList);

        for (Stack *environDependency : missing_dependencyList2)
        {
            for (Stack *stack_dependency : environDependency->get_dependencies())
            {
                if (containsStack(stack_dependency))
                    environment_graph.add_new_edge(stack_dependency->get_name(), environDependency->get_name());
            }

            int graph_edges = environment_graph.get_graph_edges(environDependency->get_name());
            int dep_num = environDependency->get_dependency_number();

            if (graph_edges == dep_num)
                missing_stackList.erase(environDependency);
        }
    }

    void print_missing_dependencies()
    {
        if (missing_stackList.size() == 0)
        {
            cout << "no dependencies missing" << endl;
            return;
        }

        // Εύρεση των dependency του περιβάλλοντος
        for (Stack *stack0 : missing_stackList)
        {
            int graph_arrows = environment_graph.get_graph_edges(stack0->get_name());
            int dep_num = stack0->get_dependency_number();

            for (Stack *stack1 : stack0->get_dependencies())
            {
                if (!containsStack(stack1))
                {
                    cout << stack0->get_name() << " dependency " << stack1->get_name() << " missing" << endl;
                }
            }
        }
    }

    // Ελέγχει αν όλα τα dependency υπάρχουν στο περιβάλλον
    bool dependencies_satisfied()
    {
        fix_graph();
        if (missing_stackList.size() == 0)
            return true;

        return false;
    }

    void serialize_dependency_order()
    {
        stackList = environment_graph.topological_sorting();
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
        for (Stack *envDependency : stackList)
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

        // Διαγραφή δεικτών cJSON
        delete root, env_array, env_name, stacks_array;
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
        std::advance(it, index);
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
    Environment *create_environment(string json_file)
    {
        cJSON *root = cJSON_Parse(json_file.c_str());
        cJSON *json_stacks;
        cJSON *json_environment_name;
        cJSON *stack_dependencies;
        cJSON *json_environment;
        cJSON *dependency;

        Stack *stack;

        json_stacks = cJSON_GetArrayItem(root, 0);
        int dep_num = cJSON_GetArraySize(json_stacks);

        json_environment = cJSON_GetArrayItem(json_stacks, 0);

        json_environment_name = cJSON_GetObjectItem(json_environment, "name");

        // Εύρεση του ονόματος του περιβάλλοντος
        string environ_name = json_environment_name->valuestring;

        stack_dependencies = cJSON_GetObjectItem(json_environment, "stacks");

        int total_stacks = cJSON_GetArraySize(stack_dependencies);

        Environment *environment = new Environment(environ_name);

        // Εύρεση των stack στο περιβάλλον και προσθήκη στη λίστα
        for (int j = 0; j < total_stacks; j++)
        {
            dependency = cJSON_GetArrayItem(stack_dependencies, j);

            stack = stack_map[dependency->valuestring];

            environment->add_stack(stack);
        }

        // Ελέγχει αν λοίπουν ακμές από τον γράφο και τις προσθέτει
        environment->fix_graph();

        // Διαγραφή δεικτών cJSON
        delete root, json_stacks, json_environment_name, stack_dependencies, json_environment, dependency;

        // Επιστροφή του περιβάλλοντος
        return environment;
    }
};

// Φόρτωση των stack από αρχείο json σε αντικείμενο Stacks
Stacks *load_stacks(string filename)
{
    Stacks *stacks = new Stacks();
    Stack *stack_object;
    Stack *stack_dependency;

    list<Stack *> dependency_list;

    cJSON *root = cJSON_Parse(filename.c_str());
    cJSON *stack_objects;
    cJSON *stack_object_json;
    cJSON *stack_name_json;
    cJSON *dependencies;
    cJSON *dependency;

    stack_objects = cJSON_GetArrayItem(root, 0);
    int stack_num = cJSON_GetArraySize(stack_objects);

    for (int i = 0; i < stack_num; i++)
    {
        stack_object_json = cJSON_GetArrayItem(stack_objects, i);
        stack_name_json = cJSON_GetObjectItem(stack_object_json, "name");

        // Εύρεση του ονόματος του stack
        string stack_name = stack_name_json->valuestring;

        dependencies = cJSON_GetObjectItem(stack_object_json, "dependencies");

        int dependency_num = cJSON_GetArraySize(dependencies);

        // Εύρεση των dependency του stack
        for (int j = 0; j < dependency_num; j++)
        {
            dependency = cJSON_GetArrayItem(dependencies, j);

            stack_dependency = stacks->get_stack_by_name(dependency->valuestring);

            dependency_list.push_back(stack_dependency);
        }

        // Δημιουργία νέου αντικειμένου stack
        stack_object = new Stack(stack_name, dependency_list);

        // Διαγραφή όλων των στοιχείων από τη λίστα
        dependency_list.clear();

        // Προσθήκη του stack στην κλάση stacks
        stacks->add_new_stack(stack_name, stack_object);
    }

    // Διαγραφή δεικτών cJSON
    delete root, stack_objects, stack_object_json, stack_name_json, dependencies, dependency;

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

            // cin >> inputfile; // remove
            inputfile = "env1.txt";

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
            environment->add_stack(dep);
            environment->fix_graph();

            cout << "\ndependencies: " << endl;
            environment->print_dependencyList();
            cout << endl;
        }

        else if (input == 3 && environment)
        {
            environment->print_missing_dependencies();
            cout << endl;
        }

        else if (input == 4 && environment)
        {
            if (environment->dependencies_satisfied())
            {
                environment->serialize_dependency_order();
                cout << "\ndependencies: " << endl;
                environment->print_dependencyList();
                cout << endl;
            }
            else
                cout << "provide all dependencies first" << endl
                     << endl;
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
