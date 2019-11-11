#include <iostream>
#include <atomic>
#include <deque>
#include <mutex>
#include <memory>
#include <thread>
#include <vector>

template<typename T>
class coarse_stack
{
private:
	std::deque<T> s;
	mutable std::mutex m;
public:
	coarse_stack() {
		s.resize(0);
	}
	void push(T item) {
		std::lock_guard<std::mutex> lk(m);
		s.push_back(std::move(item));
	}
};

template<typename T>
class fine_stack
{
private:
    struct node
    {
        T data;
        node* next;
        node(T const& data_) : data(data_), next(nullptr){}
    };
    std::unique_ptr<node> head;
    std::mutex m;
public:
	void push(T item) {

		std::unique_ptr<node> new_node = std::make_unique<node>(std::move(item));
		std::lock_guard<std::mutex> lk(m);
		new_node->next = head.get();
		head = std::move(new_node);
	}
};

template<typename T>
class lock_free_stack
{
private:
    struct node
    {
        T data;
        node* next;
        node(T const& data_):data(data_), next(nullptr){}
    };
    std::atomic<node*> head;
public:

    // verbose push
    void push(T const& data)
    {
        node* const new_node=new node(data);
        new_node->next=head.load(); // 1)
        while(true) {
            if(!head.compare_exchange_weak(new_node->next,new_node)) { // 2
                continue; // 3
            } else {
                break; // 4
            }
        }
    }
};

namespace {
coarse_stack<std::vector<int>> cs;
fine_stack<std::vector<int>> fs;
lock_free_stack<std::vector<int>> lfs;
}

void pusher(const int count) {
	for(int i = 0; i < count; i++) {
		std::vector<int> t;
		for(int j = 0; j < 1000; j++) {
		    t.push_back(j);
		}
		cs.push(t);
	}
}


#include <chrono>

class Timer
{
public:
    Timer() : beg_(clock_::now()) {}
    void reset() { beg_ = clock_::now(); }
    double elapsed() const {
        return std::chrono::duration_cast<second_>
            (clock_::now() - beg_).count(); }

private:
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<double, std::ratio<1> > second_;
    std::chrono::time_point<clock_> beg_;
};


int main(int argc, const char * argv[]) {
    Timer t;

	int number_of_threads = 4;
	int problem_size = 210000;

	int items_per_thread = problem_size / number_of_threads;

    std::thread t1(pusher, items_per_thread);
    std::thread t2(pusher, items_per_thread);
    std::thread t3(pusher, items_per_thread);
    std::thread t4(pusher, items_per_thread);
	/*
    std::thread t5(pusher, items_per_thread);
    std::thread t6(pusher, items_per_thread);
    std::thread t7(pusher, items_per_thread);
	*/
    t1.join();
    t2.join();
    t3.join();
    t4.join();
	/*
    t5.join();
    t6.join();
    t7.join();
	*/
    std::cout << t.elapsed() << std::endl;
    std::cout << "Hello, World!\n";
	std::cin.get();
    return 0;
}

