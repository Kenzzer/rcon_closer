#pragma once

#include <smsdk_ext.h>

class RCONCloser : public SDKExtension
{
public:
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late) override;
	virtual void SDK_OnUnload() override;
};