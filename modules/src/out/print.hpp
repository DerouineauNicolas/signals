#pragma once

#include "internal/module.hpp"
#include <string>

using namespace Modules;

namespace Out {

class Print : public Module {
public:
	static Print* create(std::ostream &os);
	bool process(std::shared_ptr<Data> data);
	bool handles(const std::string &url);
	static bool canHandle(const std::string &url);

private:
	Print& operator= (const Print&) = delete;
	Print(std::ostream &os);

	std::ostream &os;
};

}
