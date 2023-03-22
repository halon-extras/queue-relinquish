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
	time_t now = time(nullptr);

	std::lock_guard<std::mutex> lock(slock);
	for (auto skip = skips.begin(); skip != skips.end(); )
	{
		if (skip->ttl < now)
		{
			skip = skips.erase(skip);
			continue;
		}

		bool match = true;
		size_t o = 0;
		for (auto z : {
				HALONMTA_MESSAGE_TRANSPORTID,
				HALONMTA_MESSAGE_LOCALIP,
				HALONMTA_MESSAGE_REMOTEIP,
				HALONMTA_MESSAGE_REMOTEMX,
				HALONMTA_MESSAGE_RECIPIENTDOMAIN,
				HALONMTA_MESSAGE_JOBID,
				HALONMTA_MESSAGE_GROUPING
				})
		{
			if (skip->fields & (1 << z))
			{
				char* v;
				size_t vlen;
				HalonMTA_message_getinfo(hqm, z, nullptr, 0, &v, &vlen);
				if (skip->data[o] != std::string(v, vlen))
				{
					match = false;
					break;
				}
				++o;
				if (o >= skip->data.size())
					break;
			}
		}

		if (match)
			return false;
		++skip;
	}
	return true;
}

void queue_relinquish(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	HalonHSLValue* arg = HalonMTA_hsl_argument_get(args, 0);
	if (!arg || HalonMTA_hsl_value_type(arg) != HALONMTA_HSL_TYPE_ARRAY)
	{
		HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "Bad or missing fields argument", 0);
		return;
	}

	HalonHSLValue* arg2 = HalonMTA_hsl_argument_get(args, 1);
	if (!arg2 || HalonMTA_hsl_value_type(arg2) != HALONMTA_HSL_TYPE_NUMBER)
	{
		HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "Bad or missing ttl argument", 0);
		return;
	}

	double ttl;
	HalonMTA_hsl_value_get(arg2, HALONMTA_HSL_TYPE_NUMBER, &ttl, nullptr);

	size_t index = 0;
	HalonHSLValue *k, *v;

	skip s;
	while ((v = HalonMTA_hsl_value_array_get(arg, index, &k)))
	{
		if (HalonMTA_hsl_value_type(k) != HALONMTA_HSL_TYPE_STRING)
		{
			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "fields[] key is not a string", 0);
			return;
		}
		if (HalonMTA_hsl_value_type(v) != HALONMTA_HSL_TYPE_STRING)
		{
			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "fields[] value is not a string", 0);
			return;
		}

		char *string, *stringv;
		size_t stringl, stringvl;
		HalonMTA_hsl_value_get(k, HALONMTA_HSL_TYPE_STRING, &string, &stringl);
		HalonMTA_hsl_value_get(v, HALONMTA_HSL_TYPE_STRING, &stringv, &stringvl);
		if (std::string(string, stringl) == "transportid")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_TRANSPORTID;
			s.data.push_back(std::string(stringv, stringvl));
		}
		else if (std::string(string, stringl) == "localip")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_LOCALIP;
			s.data.push_back(std::string(stringv, stringvl));
		}
		else if (std::string(string, stringl) == "remoteip")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_REMOTEIP;
			s.data.push_back(std::string(stringv, stringvl));
		}
		else if (std::string(string, stringl) == "remotemx")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_REMOTEMX;
			s.data.push_back(std::string(stringv, stringvl));
		}
		else if (std::string(string, stringl) == "recipientdomain")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_RECIPIENTDOMAIN;
			s.data.push_back(std::string(stringv, stringvl));
		}
		else if (std::string(string, stringl) == "jobid")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_JOBID;
			s.data.push_back(std::string(stringv, stringvl));
		}
		else if (std::string(string, stringl) == "grouping")
		{
			s.fields |= 1 << HALONMTA_MESSAGE_GROUPING;
			s.data.push_back(std::string(stringv, stringvl));
		}
		else
		{
			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "Unknown field type", 0);
			return;
		}
		++index;
	}

	HalonHSLValue* arg3 = HalonMTA_hsl_argument_get(args, 2);
	bool update = true;
	if (arg3 && HalonMTA_hsl_value_type(arg3) == HALONMTA_HSL_TYPE_ARRAY)
	{
		index = 0;
		while ((v = HalonMTA_hsl_value_array_get(arg3, index, &k)))
		{
			if (HalonMTA_hsl_value_type(k) != HALONMTA_HSL_TYPE_STRING)
			{
				HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
				HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "options[] key is not a string", 0);
				return;
			}
			char *string;
			size_t stringl;
			HalonMTA_hsl_value_get(k, HALONMTA_HSL_TYPE_STRING, &string, &stringl);
			if (std::string(string, stringl) == "update")
			{
				HalonMTA_hsl_value_get(v, HALONMTA_HSL_TYPE_BOOLEAN, &update, nullptr);
			}
			else
			{
				HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
				HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "Unknown option", 0);
				return;
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

void queue_relinquish_enabled(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	HalonMTA_hsl_value_set(ret, HALONMTA_HSL_TYPE_ARRAY, nullptr, 0);

	time_t now = time(nullptr);

	std::lock_guard<std::mutex> lock(slock);
	for (auto skip = skips.begin(); skip != skips.end(); )
	{
		if (skip->ttl < now)
		{
			skip = skips.erase(skip);
			continue;
		}

		double i = std::distance(skips.begin(), skip);

		HalonHSLValue *k, *v;
		HalonMTA_hsl_value_array_add(ret, &k, &v);
		HalonMTA_hsl_value_set(k, HALONMTA_HSL_TYPE_NUMBER, (void*)&i, 0);

		size_t o = 0;
		for (auto z : {
				HALONMTA_MESSAGE_TRANSPORTID,
				HALONMTA_MESSAGE_LOCALIP,
				HALONMTA_MESSAGE_REMOTEIP,
				HALONMTA_MESSAGE_REMOTEMX,
				HALONMTA_MESSAGE_RECIPIENTDOMAIN,
				HALONMTA_MESSAGE_JOBID,
				HALONMTA_MESSAGE_GROUPING
				})
		{
			if (skip->fields & (1 << z))
			{
				std::string key;
				switch (z) {
					case HALONMTA_MESSAGE_TRANSPORTID:
						key = "transportid";
						break;
					case HALONMTA_MESSAGE_LOCALIP:
						key = "localip";
						break;
					case HALONMTA_MESSAGE_REMOTEIP:
						key = "remoteip";
						break;
					case HALONMTA_MESSAGE_REMOTEMX:
						key = "remotemx";
						break;
					case HALONMTA_MESSAGE_RECIPIENTDOMAIN:
						key = "recipientdomain";
						break;
					case HALONMTA_MESSAGE_JOBID:
						key = "jobid";
						break;
					case HALONMTA_MESSAGE_GROUPING:
						key = "grouping";
						break;
				}
				HalonHSLValue *k2, *v2;
				HalonMTA_hsl_value_array_add(v, &k2, &v2);
				HalonMTA_hsl_value_set(k2, HALONMTA_HSL_TYPE_STRING, key.c_str(), 0);
				HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_STRING, skip->data[o].c_str(), 0);

				++o;
				if (o >= skip->data.size())
					break;
			}
		}
		++skip;
	}
}

HALON_EXPORT
int Halon_version()
{
	return HALONMTA_PLUGIN_VERSION;
}

HALON_EXPORT
bool Halon_hsl_register(HalonHSLRegisterContext* hhrc)
{
	HalonMTA_hsl_register_function(hhrc, "queue_relinquish", queue_relinquish);
	HalonMTA_hsl_register_function(hhrc, "queue_relinquish_enabled", queue_relinquish_enabled);
	HalonMTA_hsl_module_register_function(hhrc, "queue_relinquish", queue_relinquish);
	HalonMTA_hsl_module_register_function(hhrc, "queue_relinquish_enabled", queue_relinquish_enabled);
	return true;
}
