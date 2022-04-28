#include <HalonMTA.h>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <time.h>
#include <mutex>

struct skip
{
	int ttl = 0;
	int fields = 0;
	std::vector<std::string> data;
};
std::list<skip> skips;
std::mutex slock;

HALON_EXPORT
bool Halon_queue_pickup_acquire(HalonQueueMessage *hqm)
{
	std::lock_guard<std::mutex> lock(slock);
	for (const auto & skip : skips)
	{
		if (!skip.fields)
			continue;
		if (skip.ttl < time(nullptr))
			continue;

		size_t o = 0;
		for (auto z : {
				HALONMTA_MESSAGE_TRANSPORTID,
				HALONMTA_MESSAGE_LOCALIP,
				HALONMTA_MESSAGE_REMOTEIP,
				HALONMTA_MESSAGE_REMOTEMX,
				HALONMTA_MESSAGE_RECIPIENTDOMAIN,
				HALONMTA_MESSAGE_JOBID
				})
		{
			if (skip.fields & (1 << z))
			{
				char* v;
				size_t vlen;
				HalonMTA_message_getinfo(hqm, z, nullptr, 0, &v, &vlen);
				if (skip.data[o] != std::string(v, vlen))
					continue;
				++o;
				if (o >= skip.data.size())
					break;
			}
		}
		return false;
	}
	return true;
}

void queue_pickup_cancel(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	HalonHSLValue* arg = HalonMTA_hsl_argument_get(args, 0);

	size_t index = 0;
	HalonHSLValue *k, *v;

	skip s;
	while ((v = HalonMTA_hsl_value_array_get(arg, index, &k)))
	{
		char *string, *stringv;
		size_t stringl, stringvl;
		HalonMTA_hsl_value_get(k, HALONMTA_HSL_TYPE_STRING, &string, &stringl);
		HalonMTA_hsl_value_get(v, HALONMTA_HSL_TYPE_STRING, &stringv, &stringvl);
		if (std::string(string, stringl) == "transportid")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_TRANSPORTID;
			s.data.push_back(std::string(stringv, stringvl));
		}
		if (std::string(string, stringl) == "localip")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_LOCALIP;
			s.data.push_back(std::string(stringv, stringvl));
		}
		if (std::string(string, stringl) == "remoteip")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_REMOTEIP;
			s.data.push_back(std::string(stringv, stringvl));
		}
		if (std::string(string, stringl) == "remotemx")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_REMOTEMX;
			s.data.push_back(std::string(stringv, stringvl));
		}
		if (std::string(string, stringl) == "recipientdomain")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_RECIPIENTDOMAIN;
			s.data.push_back(std::string(stringv, stringvl));
		}
		if (std::string(string, stringl) == "jobid")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_JOBID;
			s.data.push_back(std::string(stringv, stringvl));
		}
		++index;
	}

	HalonMTA_hsl_value_set(ret, HALONMTA_HSL_TYPE_STRING, "key", 0);

	HalonHSLValue* arg2 = HalonMTA_hsl_argument_get(args, 1);
	double ttl;
	HalonMTA_hsl_value_get(arg2, HALONMTA_HSL_TYPE_NUMBER, &ttl, nullptr);

	HalonHSLValue* arg3 = HalonMTA_hsl_argument_get(args, 2);
	bool update = true;
	if (arg3)
	{
		while ((v = HalonMTA_hsl_value_array_get(arg3, index, &k)))
		{
			char *string;
			size_t stringl;
			HalonMTA_hsl_value_get(k, HALONMTA_HSL_TYPE_STRING, &string, &stringl);
			if (std::string(string, stringl) == "update")
			{
				HalonMTA_hsl_value_get(v, HALONMTA_HSL_TYPE_BOOLEAN, &update, nullptr);
			}
			++index;
		}
	}

	std::lock_guard<std::mutex> lock(slock);
	for (auto & skip : skips)
	{
		if (skip.fields == s.fields && skip.data == s.data)
		{
			if (update)
				skip.ttl = time(nullptr) + ttl;
			return;
		}
	}
	s.ttl = time(nullptr) + ttl;
	skips.push_back(s);
}

HALON_EXPORT
int Halon_version()
{
	return HALONMTA_PLUGIN_VERSION;
}

HALON_EXPORT
bool Halon_hsl_register(HalonHSLRegisterContext* hhrc)
{
	HalonMTA_hsl_register_function(hhrc, "queue_pickup_cancel", queue_pickup_cancel);
	return true;
}
