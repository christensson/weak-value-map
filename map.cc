#include <node.h>
#include <node_object_wrap.h>
#include <unordered_map>
#include <cstdint>

struct WeakValue;

typedef std::pair<std::unordered_map<std::string, WeakValue>*, std::string> MapKeyPair;
typedef std::uint64_t InstanceId_t;
typedef std::unordered_map<InstanceId_t, bool> InstanceIdMap;

struct FinalizationCbData {
	InstanceId_t weakValueMapInstanceId;
	MapKeyPair* pair;

	FinalizationCbData(InstanceId_t weakValueMapInstanceId, MapKeyPair* pair)
		: weakValueMapInstanceId(weakValueMapInstanceId), pair(pair) {
	}
	~FinalizationCbData() {}
};

struct WeakValue {
	MapKeyPair* pair = nullptr;
	v8::UniquePersistent<v8::Value> value;
	WeakValue(std::unordered_map<std::string, WeakValue>* m, std::string s, v8::UniquePersistent<v8::Value>&& v)
		: value(std::move(v)) {
		this->pair = new MapKeyPair(m, s);
	}
	~WeakValue() {
		if (this->pair != nullptr) {
			delete this->pair;
			this->pair = nullptr;
		}
	}
	WeakValue() { }
	WeakValue(WeakValue&& o) {
		std::swap(this->pair, o.pair);
		this->value = std::move(o.value);
	}
};

class WeakValueMap : public node::ObjectWrap {
public:
	static void Init(v8::Local<v8::Object> exports) {
		v8::Isolate* isolate = exports->GetIsolate();

		//Create constructor funtion template
		v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
		tpl->SetClassName(v8::String::NewFromUtf8(isolate, "WeakValueMap"));
		tpl->InstanceTemplate()->SetInternalFieldCount(1);

		//Set class prototype methods
		NODE_SET_PROTOTYPE_METHOD(tpl, "set", Set);
		NODE_SET_PROTOTYPE_METHOD(tpl, "delete", Delete);
		NODE_SET_PROTOTYPE_METHOD(tpl, "get", Get);

		constructor.Reset(isolate, tpl->GetFunction());
		exports->Set(v8::String::NewFromUtf8(isolate, "WeakValueMap"), tpl->GetFunction());

		WeakValueMap::instanceId = 0;
		WeakValueMap::instanceMap = new InstanceIdMap();
	}

private:
	explicit WeakValueMap() {
		this->map = new std::unordered_map<std::string, WeakValue>;
		this->id = WeakValueMap::instanceId++;
		WeakValueMap::instanceMap->insert(std::make_pair(this->id, true));
	}

	~WeakValueMap() {
		delete this->map;
		this->map = nullptr;
		WeakValueMap::instanceMap->erase(this->id);
	}

	static void New(const v8::FunctionCallbackInfo<v8::Value>& args) {
		//Make sure this is a construct or throw a type error
		if (args.IsConstructCall()) {
			WeakValueMap* obj = new WeakValueMap();
			obj->Wrap(args.This());
			args.GetReturnValue().Set(args.This());
		} else {
			v8::Isolate* isolate = args.GetIsolate();
			isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Constructor WeakValueMap required 'new'")));
		}
	}

	static void finalizationCb(const v8::WeakCallbackInfo<FinalizationCbData>& data) {
		auto d = data.GetParameter();
		auto id = d->weakValueMapInstanceId;
		// Check that WeakValueMap hasn't been deleted
		if (WeakValueMap::instanceMap->count(id) != 0) {
			auto p = d->pair;
			p->first->erase(p->second);
		}
		delete d;
	};

	static void Set(const v8::FunctionCallbackInfo<v8::Value>& args) {
		v8::Isolate* isolate = args.GetIsolate();

		WeakValueMap* obj = ObjectWrap::Unwrap<WeakValueMap>(args.Holder());

		//Delete from the map if the value to insert is undefined
		if (args[1]->IsUndefined()) return WeakValueMap::Delete(args);

		std::string key = std::string(*v8::String::Utf8Value(args[0]->ToString()));
		v8::UniquePersistent<v8::Value> value(isolate, args[1]);

		WeakValue val(obj->map, key, std::move(value));
		FinalizationCbData* cbData = new FinalizationCbData(obj->id, val.pair);
		val.value.SetWeak(cbData, WeakValueMap::finalizationCb, v8::WeakCallbackType::kParameter);
		obj->map->insert(std::make_pair(key, std::move(val)));

		args.GetReturnValue().Set(args.Holder());
	}

	static void Delete(const v8::FunctionCallbackInfo<v8::Value>& args) {
		WeakValueMap* obj = ObjectWrap::Unwrap<WeakValueMap>(args.Holder());

		std::string key = std::string(*v8::String::Utf8Value(args[0]->ToString()));
		obj->map->erase(key);

		args.GetReturnValue().Set(args.Holder());
	}

	static void Get(const v8::FunctionCallbackInfo<v8::Value>& args) {
		WeakValueMap* obj = ObjectWrap::Unwrap<WeakValueMap>(args.Holder());

		std::string key = std::string(*v8::String::Utf8Value(args[0]->ToString()));

		if (obj->map->count(key) == 1)
			args.GetReturnValue().Set((*(obj->map))[key].value);
	}

	static v8::Persistent<v8::Function> constructor;
	static InstanceId_t instanceId;
	static InstanceIdMap* instanceMap;

	InstanceId_t id;
	std::unordered_map<std::string, WeakValue>* map;
};

v8::Persistent<v8::Function> WeakValueMap::constructor;

InstanceIdMap* WeakValueMap::instanceMap;
InstanceId_t WeakValueMap::instanceId;

NODE_MODULE(addon, WeakValueMap::Init)
