#pragma once

#include "allocator.hpp"
#include "data.hpp"
#include "metadata.hpp"
#include "lib_modules/utils/helper.hpp"
#include "lib_utils/log.hpp"
#include "lib_utils/tools.hpp"
#include <lib_signals/signals.hpp>
#include <thread>


namespace Modules {

using namespace Signals;

typedef Signal<void(std::shared_ptr<const Data>), ResultQueue<NotVoid<void>>> SignalAsync;
typedef Signal<void(std::shared_ptr<const Data>), ResultVector<NotVoid<void>>> SignalSync;

typedef SignalSync SignalDefaultSync;

struct IOutput {
	virtual ~IOutput() noexcept(false) {}
	virtual size_t emit(std::shared_ptr<const Data> data) = 0;
	virtual ISignal<void(std::shared_ptr<const Data>)>& getSignal() = 0;
};

inline size_t ConnectOutput(IOutput* p, std::function<void(std::shared_ptr<const Data>)> functor) {
	return p->getSignal().connect(functor);
}

template<typename C, typename D>
size_t ConnectOutput(IOutput* p, C ObjectSlot, D MemberFunctionSlot) {
	auto functor = MEMBER_FUNCTOR(ObjectSlot, MemberFunctionSlot);
	return ConnectOutput(p, functor);
}

inline size_t ConnectOutput(IOutput* p, std::function<void(std::shared_ptr<const Data>)> functor, IProcessExecutor& executor) {
	return p->getSignal().connect(functor, executor);
}

template<typename C, typename D, typename E>
size_t ConnectOutput(IOutput* p, C ObjectSlot, D MemberFunctionSlot, E& executor) {
	auto functor = MEMBER_FUNCTOR(ObjectSlot, MemberFunctionSlot);
	return ConnectOutput(p, functor, executor);
}

template<typename Allocator, typename Signal>
class OutputT : public IOutput, public Metadata {
public:
	typedef Allocator AllocatorType;

	OutputT(IIMetadata *metadata = nullptr)
		: Metadata(metadata), allocator(new Allocator) {
	}

	~OutputT() noexcept(false) {
		allocator->unblock();
	}

	size_t emit(std::shared_ptr<const Data> data) {
		updateMetadata(data);
		size_t numReceivers = signal.emit(data);
		if (numReceivers == 0)
			Log::msg(Log::Debug, "emit(): Output had no receiver");
		return numReceivers;
	}

	template<typename T = typename Allocator::MyType>
	std::shared_ptr<T> getBuffer(size_t size) {
		return allocator->getBuffer<T>(size);
	}

	ISignal<void(std::shared_ptr<const Data>)>& getSignal() override {
		return signal;
	}

	//Takes ownership.
	void setAllocator(Allocator *allocator) {
		this->allocator = uptr(allocator);
	}

private:
	Signal signal;
	std::unique_ptr<Allocator> allocator;
};

template<typename DataType> using OutputDataDefault = OutputT<PacketAllocator<DataType>, SignalDefaultSync>;

typedef OutputDataDefault<RawData> OutputDefault;

}