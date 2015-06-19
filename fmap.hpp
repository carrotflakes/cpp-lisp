#include <vector>
#include <utility>
#include <algorithm>

template <typename T0, typename T1>
class Fmap {
	std::vector<std::pair<T0, T1> > entries;

public:
	typedef typename std::vector<std::pair<T0, T1> >::iterator iterator;
	typedef typename std::vector<std::pair<T0, T1> >::const_iterator const_iterator;

	Fmap() {}

	int count(const T0 key) const {
		int low = 0, up = entries.size(), i;
		if (up == 0)
			return 0;
		while (low < up) {
			i = (low + up) / 2;
			if (entries[i].first == key)
				return 1;
			if (key < entries[i].first)
				up = i;
			else
				low = i + 1;
		}
		return 0;
	}

	const T1 &operator[] (const T0 key) const {
		int low = 0, up = entries.size(), i;
		if (up == 0)
			throw "no entry";
		while (low < up) {
			i = (low + up) / 2;
			if (entries[i].first == key)
				return entries[i].second;
			if (key < entries[i].first)
				up = i;
		  else
				low = i + 1;
		}
		throw "no entry";
	}

	T1 &operator[] (const T0 key) {
		int low = 0, up = entries.size(), i;
		if (up == 0) {
			entries.push_back(std::pair<T0, T1>(key, T1()));
			return entries[0].second;
		}
		while (low < up) {
			i = (low + up) / 2;
			if (entries[i].first == key)
				return entries[i].second;
			if (key < entries[i].first)
				up = i;
			else
				low = i + 1;
		}
			auto it = entries.begin();
			std::advance(it, low);
			it = entries.insert(it, std::pair<T0, T1>(key, T1()));
			return it->second;
	}

	iterator begin() { return entries.begin(); }
	iterator end() { return entries.end(); }
	const_iterator begin() const { return entries.begin(); }
	const_iterator end() const { return entries.end(); }
};
