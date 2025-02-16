#ifndef BleTypes_h
#define BleTypes_h

/* //////////////////////////////////////////////////////////////
*
* BleTypes
*
* Created: Chip Audette, OpenAudio
*
* Purpose: Custom data types for handling BLE data
*
* MIT License, Use at your own risk.
*
////////////////////////////////////////////////////////////// */

#include <vector>
#include <queue>

typedef struct {
	int service_id = -1;
	int characteristic_id = -1;
	std::vector<uint8_t> data;
} BleDataMessage;

class BleDataMessageQueue {
	public:
		BleDataMessageQueue() {};
		
		int push(const BleDataMessage &msg) {
			if (queue.size() == max_allowed_size) {
				return -1;
			}
			queue.push(msg);
			return 0;
		}
		
		BleDataMessage front(void) { return queue.front(); }
		void pop(void) { queue.pop(); }
		size_t size(void) { return queue.size(); }
		
		uint32_t getMaxAllowedSize(void) { return max_allowed_size; }
		uint32_t setMaxAllowedSize(uint32_t size) { return max_allowed_size = size; }
		
	protected:
		uint32_t max_allowed_size = 16;
		std::queue<BleDataMessage> queue;
};

#endif