#include <cstdio>
#include <cstdlib>   // malloc, free
#include <iostream>

// Node structure
template <typename T>
struct node
{
    T data;          // stored value
    node<T> *next;   // pointer to next node
};

// Linked list structure (head and tail)
template <typename T>
struct linked_list
{
    node<T> *first;  // pointer to first node
    node<T> *last;   // pointer to last node
};

// Allocate and initialize a new linked list
template <typename T>
linked_list<T> *new_linked_list()
{
    linked_list<T> *list = (linked_list<T> *) malloc(sizeof(linked_list<T>));
    list->first = nullptr;
    list->last = nullptr;
    return list;
}

// Append a node with value `data` to the tail of the list
template <typename T>
void add_node(linked_list<T> *list, T data)
{
    node<T> *new_node = (node<T> *) malloc(sizeof(node<T>));
    new_node->data = data;
    new_node->next = nullptr;

    if (list->first == nullptr) // empty list
    {
        list->first = new_node;
        list->last = new_node;
    }
    else
    {
        list->last->next = new_node;
        list->last = new_node;
    }
}

// Insert a node with value `data` at position `position` (0-based).
// If position <= 0, insert at head. If position >= length, insert at tail.
template <typename T>
void insert_at(linked_list<T> *list, int position, T data)
{
    node<T> *new_node = (node<T> *) malloc(sizeof(node<T>));
    new_node->data = data;
    new_node->next = nullptr;

    if (position <= 0 || list->first == nullptr)
    {
        new_node->next = list->first;
        list->first = new_node;
        if (list->last == nullptr)
            list->last = new_node;
        return;
    }

    node<T> *current = list->first;
    int index = 0;
    while (current->next != nullptr && index < position - 1)
    {
        current = current->next;
        index++;
    }

    new_node->next = current->next;
    current->next = new_node;

    if (new_node->next == nullptr)
        list->last = new_node;
}

// Delete node at position `position` (0-based).
// If position <= 0, delete head. If position out of range, no-op.
template <typename T>
void delete_at(linked_list<T> *list, int position)
{
    if (list->first == nullptr) return; // empty

    node<T> *to_delete = nullptr;

    if (position <= 0)
    {
        to_delete = list->first;
        list->first = list->first->next;
        if (list->first == nullptr)
            list->last = nullptr;
        free(to_delete);
        return;
    }

    node<T> *current = list->first;
    int index = 0;
    while (current->next != nullptr && index < position - 1)
    {
        current = current->next;
        index++;
    }

    if (current->next == nullptr) return; // out of range

    to_delete = current->next;
    current->next = to_delete->next;
    if (to_delete == list->last)
        list->last = current;
    free(to_delete);
}

// Print all elements from head to tail
template <typename T>
void traverse_list(linked_list<T> *list)
{
    node<T> *current = list->first;
    while (current != nullptr)
    {
        std::cout << current->data << " ";
        current = current->next;
    }
    std::cout << std::endl;
}

// Free every node and then free the list container itself
template <typename T>
void delete_linked_list(linked_list<T> *list)
{
    node<T> *current = list->first;
    while (current != nullptr)
    {
        node<T> *next = current->next;
        free(current);
        current = next;
    }
    list->first = nullptr;
    list->last = nullptr;
    free(list);
}

// Main: interactive menu calling only the functions defined above.
// Only <cstdio>, <cstdlib>, <iostream> are used.
int main()
{
    linked_list<int> *list = new_linked_list<int>();
    bool running = true;

    while (running)
    {
        std::cout << "\n=== Menu ===\n";
        std::cout << "1. append to tail (add_node)\n";
        std::cout << "2. insert at position (insert_at)\n";
        std::cout << "3. delete at position (delete_at)\n";
        std::cout << "4. traverse and print (traverse_list)\n";
        std::cout << "5. delete entire list and exit (delete_linked_list)\n";
        std::cout << "0. exit without freeing (may leak)\n";
        std::cout << "Choose option: ";

        int option;
        if (!(std::cin >> option))
        {
            std::cin.clear();
            std::cin.ignore(10000, '\n'); // discard invalid input
            std::cout << "Invalid input, please enter a number.\n";
            continue;
        }

        switch (option)
        {
            case 1:
            {
                std::cout << "Enter integer to append: ";
                int value;
                if (!(std::cin >> value))
                {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    std::cout << "Invalid integer.\n";
                }
                else
                {
                    add_node(list, value); // uses only defined function
                    std::cout << "Appended.\n";
                }
                break;
            }

            case 2:
            {
                std::cout << "Enter insertion index (0 = head): ";
                int pos;
                if (!(std::cin >> pos))
                {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    std::cout << "Invalid index.\n";
                    break;
                }
                std::cout << "Enter integer to insert: ";
                int value;
                if (!(std::cin >> value))
                {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    std::cout << "Invalid integer.\n";
                    break;
                }
                insert_at(list, pos, value); // uses only defined function
                std::cout << "Inserted.\n";
                break;
            }

            case 3:
            {
                std::cout << "Enter deletion index (0 = head): ";
                int pos;
                if (!(std::cin >> pos))
                {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    std::cout << "Invalid index.\n";
                    break;
                }
                delete_at(list, pos); // uses only defined function
                std::cout << "Deleted (if index valid).\n";
                break;
            }

            case 4:
            {
                std::cout << "List contents: ";
                traverse_list(list); // uses only defined function
                break;
            }

            case 5:
            {
                delete_linked_list(list); // uses only defined function
                std::cout << "List freed. Exiting.\n";
                running = false;
                break;
            }

            case 0:
            {
                std::cout << "Exiting without freeing list. (Memory not freed)\n";
                running = false;
                break;
            }

            default:
            {
                std::cout << "Invalid option.\n";
                break;
            }
        }
    }

    return 0;
}
