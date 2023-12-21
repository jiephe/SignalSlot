#include <boost/asio.hpp>
#include <iostream>
#include <functional>
#include <vector>
#include <memory>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <thread>

pid_t getThreadId() {
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

template<typename... Args>
class BaseSignal {
public:
    using SlotFunction = std::function<void(Args...)>;

    struct SlotInfo {
    public:
        SlotFunction slot_func_;
        boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    public:
        SlotInfo(std::shared_ptr<boost::asio::io_context> io_context, SlotFunction func)
            : strand_(boost::asio::make_strand(*io_context)), slot_func_(func) {}

        void post_task(Args... args) {
            boost::asio::post(strand_, std::bind(slot_func_, std::move(args)...));
        }
    };

    void connect(std::shared_ptr<boost::asio::io_context> io_context, SlotFunction func) {
        slots_.emplace_back(io_context, func);
    }

    void operator()(Args... args) {
        for (auto& si : slots_) {
            //std::cout << "post tid: " << getThreadId() << std::endl;
            si.post_task(std::move(args)...);
        }
    }    

private:
    std::vector<SlotInfo> slots_;
};

template<typename ObjectType, typename SignalType, typename FunctionObjectType, typename FunctionType>
void connect(ObjectType* sender, SignalType ObjectType::*signalMemberPtr, FunctionObjectType* receiver, FunctionType function) {
    std::shared_ptr<boost::asio::io_context> io_context = receiver->get_io_context();

    assert(io_context != nullptr);
    
    (sender->*signalMemberPtr).connect(io_context, [receiver, function](auto&&... args) {
        (receiver->*function)(std::forward<decltype(args)>(args)...);
    });
}

class SlotThread {
public:
    SlotThread() : io_context_(std::make_shared<boost::asio::io_context>()), work_guard_(io_context_->get_executor()) {}

    void io_run() {
        std::cout << "io run tid: " << getThreadId() << std::endl;
        io_context_->run();
    }

    std::shared_ptr<boost::asio::io_context> get_io_context() {
        return io_context_;
    }

    void start() {
        runner_.reset(new std::thread([this]{this->io_run();}));
    }

private:
    std::shared_ptr<boost::asio::io_context> io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    std::shared_ptr<std::thread> runner_;
};

class BaseSlot {
public:
    static void setMainIOContext(std::shared_ptr<boost::asio::io_context> io_context) {
        main_io_context_ = io_context;
    }

    explicit BaseSlot(std::shared_ptr<boost::asio::io_context> io_context = nullptr)
        : io_context_(io_context ? io_context : main_io_context_) {
    }

    std::shared_ptr<boost::asio::io_context> get_io_context() {
        return io_context_;
    }

    void moveToThread(std::shared_ptr<SlotThread> pt) {
        io_context_ = pt->get_io_context();
    }

private:
    std::shared_ptr<boost::asio::io_context> io_context_;
    static std::shared_ptr<boost::asio::io_context> main_io_context_;
};
std::shared_ptr<boost::asio::io_context> BaseSlot::main_io_context_ = nullptr;

class CSApplication
{
public:
    CSApplication() : main_io_context_(std::make_shared<boost::asio::io_context>()), main_work_guard_(main_io_context_->get_executor()) {
        BaseSlot::setMainIOContext(main_io_context_);  
    }

public:
    void exec() {
        main_io_context_->run();
    }

    void exit() {
        main_work_guard_.reset();
    }

private:
    std::shared_ptr<boost::asio::io_context> main_io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> main_work_guard_;
};
