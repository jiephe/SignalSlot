#include "signal_slot.hpp"

class TestStruct {
public:
    TestStruct():a(100), b(300.0), c("TestStruct") {}

    int a;
    double b;
    std::string c;
public:
    void dump() {
        std::cout << " TestStruct dump a: " << a << " b: " << b << " c: " << c << std::endl;
    }
};

class TestStruct2 {
public:
    TestStruct2():d(800), e(500.0), f("TestStruct2") {}    
    int d;
    double e;
    std::string f; 
public:
    void dump() {
        std::cout << " TestStruct2 dump d: " << d << " e: " << e << " f: " << f << std::endl;
    }
};

class TestReceiver : public BaseSlot {
public:
    explicit TestReceiver(std::shared_ptr<boost::asio::io_context> io_context = nullptr) : BaseSlot(io_context) {}

    void slot_0(const std::string& content) {
        std::cout << "slot_0 tid: " << getThreadId() << " content: " << content << std::endl;
    }

    void slot_1(std::shared_ptr<TestStruct> c1, std::shared_ptr<TestStruct2> c2, int i1) {
        std::cout << "slot_1 tid: " << getThreadId() << " c1.c: " << c1->c << " c2.f: " << c2->f << " i1: " << i1 << std::endl;
    }
};

//同步发送，槽函数再主线程里执行
class SyncSlot {
public:
    SyncSlot() {}

public:
    void set_signal() {
        TestReceiver cr;
        connect(this, &SyncSlot::signal_0, &cr, &TestReceiver::slot_0);
        connect(this, &SyncSlot::signal_1, &cr, &TestReceiver::slot_1);
    }

    void emit() {
        auto a1 = std::make_shared<TestStruct>();
        auto a2 = std::make_shared<TestStruct2>();
        signal_0("fdafdasfas");
        signal_1(a1, a2, 600);
    }

public:
    BaseSignal<const std::string&> signal_0;
    BaseSignal<std::shared_ptr<TestStruct>, std::shared_ptr<TestStruct2>, int>     signal_1;
};  

class AsyncSlot {
public:
    AsyncSlot() {}

public:
    void set_signal() {
        slot_thread_ = std::make_shared<SlotThread>();
        slot_thread_->start();

        TestReceiver cr;
        cr.moveToThread(slot_thread_);

        connect(this, &AsyncSlot::signal_0, &cr, &TestReceiver::slot_0);
        connect(this, &AsyncSlot::signal_1, &cr, &TestReceiver::slot_1);
    }

    void emit() {
        auto a1 = std::make_shared<TestStruct>();
        auto a2 = std::make_shared<TestStruct2>();
        signal_0("fdafdasfas");
        signal_1(a1, a2, 600);
    }

public:
    std::shared_ptr<SlotThread> slot_thread_;
    BaseSignal<const std::string&> signal_0;
    BaseSignal<std::shared_ptr<TestStruct>, std::shared_ptr<TestStruct2>, int>     signal_1;
};  

int main() {
    std::cout << "main thread tid: " << getThreadId() << std::endl;
    CSApplication app;

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    SyncSlot ss;
    ss.set_signal();
    ss.emit();

    AsyncSlot as;
    as.set_signal();
    as.emit();    

    app.exec();

    return 0;
}
