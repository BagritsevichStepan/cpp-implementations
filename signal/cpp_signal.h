#ifndef CPP_IMPLEMENTATIONS_CPP_SIGNAL_H
#define CPP_IMPLEMENTATIONS_CPP_SIGNAL_H

#include "intrusive_list/intrusive_list.h"
#include <functional>

namespace cpp::signal {

    template <typename T>
    class Signal;

    template <typename... Args>
    class Signal<void(Args...)> {
    private:
        using Slot = std::function<void(Args...)>;

    public:
        class Connection : public cpp::intrusive::ListElement<class ConnectionTag> {
        private:
            Connection(Signal* signal, Slot slot);

            void Replace(Connection& other);

        public:
            Connection() = default;

            Connection(const Connection& other) = delete;
            Connection& operator=(const Connection& other) = delete;

            Connection(Connection&& other);
            Connection& operator=(Connection&& other);

            void Disconnect();

            template <typename T>
            friend class Signal;

        private:
            Signal* signal_{nullptr};
            Slot slot_;

        };

    public:
        Signal() = default;

        Signal(const Signal&) = delete;
        Signal(Signal&&) = delete;
        Signal& operator=(const Signal&) = delete;
        Signal& operator=(Signal&&) = delete;

        Connection Connect(std::function<void(Args...)> slot);

        void operator()(Args... args);

        ~Signal();

    private:

        class IteratorHolder {
        public:
            explicit IteratorHolder(Signal* signal);

            ~IteratorHolder();

            template <typename T>
            friend class Signal;

        private:
            cpp::intrusive::List<Connection, ConnectionTag>::const_iterator current_;
            IteratorHolder* next_;
            const Signal* signal_;
        };

    private:
        intrusive::List<Connection, ConnectionTag> connections_{};
        mutable IteratorHolder* top_{nullptr};

    };


    // Implementation
    template <typename... Args>
    Signal<void(Args...)>::Connection Signal<void(Args...)>::Connect(std::function<void(Args...)> slot) {
        return Connection(this, std::move(slot));
    }

    template <typename... Args>
    void Signal<void(Args...)>::operator()(Args... args) {
        IteratorHolder holder(this);
        while (holder.current_ != connections_.end()) {
            auto copy = holder.current_;
            holder.current_++;
            copy->slot_(std::forward<Args>(args)...);
            if (holder.signal_ == nullptr) {
                return;
            }
        }
    }

    template <typename... Args>
    Signal<void(Args...)>::~Signal() {
        for (auto it = top_; it != nullptr; it = it->next_) {
            if (it->current_ != connections_.cend()) {
                it->signal_ = nullptr;
            }
        }

        while (!connections_.IsEmpty()) {
            connections_.Back()->signal_ = nullptr;
            connections_.PopBack();
        }
    }


    // Connection
    template <typename... Args>
    Signal<void(Args...)>::Connection::Connection(Signal* signal, std::function<void(Args...)> slot) : signal_(signal), slot_(slot) {
        signal_->connections_.PushBack(*this);
    }

    template <typename... Args>
    Signal<void(Args...)>::Connection::Connection(Signal<void(Args...)>::Connection&& other) : slot_(std::move(other.slot_)) {
        signal_ = other.signal_;
        if (other.signal_ != nullptr) {
            Replace(other);
        }
    }

    template <typename... Args>
    Signal<void(Args...)>::Connection& Signal<void(Args...)>::Connection::operator=(Signal<void(Args...)>::Connection&& other) {
        if (this != &other) {
            Disconnect();

            signal_ = other.signal_;
            slot_ = std::move(other.slot_);

            if (other.signal_) {
                Replace(other);
            }
        }
        return *this;
    }

    template<typename... Args>
    void Signal<void(Args...)>::Connection::Disconnect() {
        if (IsLinked()) {
            for (auto it = signal_->top; it != nullptr; it = it->next_) {
                if (it->current != signal_->connections.end() && &(*it->current_) == this) {
                    it->current++;
                }
            }
        }
        Unlink();
        signal_ = nullptr;
    }

    template <typename... Args>
    void Signal<void(Args...)>::Connection::Replace(Signal<void(Args...)>::Connection& other) {
        other.signal_->connections_.Insert(signal_->connections_.GetIterator(other), *this);
        other.Disconnect();
    }


    // IteratorHolder
    template <typename... Args>
    Signal<void(Args...)>::IteratorHolder::IteratorHolder(Signal* signal)
            : current_(signal->connections_.begin()), next_(signal->top_), signal_(signal) {
        signal_->top_ = this;
    }

    template <typename... Args>
    Signal<void(Args...)>::IteratorHolder::~IteratorHolder() {
        if (signal_ != nullptr) {
            signal_->top_ = next_;
        }
    }



} // End of namespace cpp::signal

#endif //CPP_IMPLEMENTATIONS_CPP_SIGNAL_H
