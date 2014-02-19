#include "pin.hpp"

namespace Modules {

//TODO: the pin could check the bool result (currently done by the allocator) and retry on failure (but is it its role?)
template<typename Allocator, typename Signal>
class PinT : public Pin {
public:
	typedef Signal SignalType;

	PinT(Props *props = nullptr)
		: props(props) {
	}

	~PinT() {
		waitForCompletion();
	}

	size_t emit(std::shared_ptr<Data> data) {
		size_t numReceivers = signal.emit(data);
		assert(numReceivers == 1);
		return numReceivers;
	}

	void waitForCompletion() {
		signal.results(); //getting the result release the shared_ptr
	}

	//TODO: this is the sync approach, where data are synced for the Pin to be destroyed.
	//      The other option is to invalidate all the data by calling
	std::shared_ptr<Data> getBuffer(size_t size) {
		for (;;) {
			std::shared_ptr<Data> data(allocator.getBuffer(size));
			if (data.get()) {
				return data;
			} else {
				signal.results(false); //see if results are ready
				data = allocator.getBuffer(size);
				if (data.get()) {
					return data;
				} else {
					signal.results(true, true); //wait synchronously for one result
					data = allocator.getBuffer(size);
					if (data.get()) {
						return data;
					} else {
#if 0
						Log::msg(Log::Error, "The allocator failed to wait for available data. Reset the whole allocator.");
						allocator.reset();
						data = allocator.getBuffer(size);
#else
						Log::msg(Log::Error, "The allocator failed to wait for available data. Add a new buffer.");
						data = allocator.getBuffer(size, true);
#endif
						assert(data.get());
						return data;
					}
				}
			}
		}
	}

	Signal& getSignal() {
		return signal;
	}

	Props* getProps() const {
		return props.get();
	}

private:
	Allocator allocator;
	Signal signal;
	std::unique_ptr<Props> props;
};

PinDefaultFactory::PinDefaultFactory() {
}

Pin* PinDefaultFactory::createPin(Props *props) {
	return new PinDefault(props);
}

}