#include "utils/CSingleton.hpp"
#include <atomic>
#include <condition_variable>
#include <string>
#include <boost/circular_buffer.hpp>
#include <ICanDevice.hpp>

constexpr size_t MAX_CAN_FRAME_DATA_LEN = 8;

#pragma pack(push, 1)
class CanData
{
public:
    CanData(uint32_t frame_id_, uint8_t data_len, uint8_t* data_)
        : frame_id(frame_id_), data_len(data_len)
    {
        memset(data, 0, sizeof(data));
        if(data_len > sizeof(data))
            data_len = sizeof(data);

        if(data_)
            memcpy(data, data_, data_len);
    }
    uint32_t frame_id;
    uint8_t data_len;
    uint8_t data[MAX_CAN_FRAME_DATA_LEN];
};
#pragma pack(pop)

/* TODO: create asbtraction for this & SerialPort because it's the same - but no time currently */
class CallbackAsyncSerial;
class CanSerialPort : public CSingleton < CanSerialPort >
{
    friend class CSingleton < CanSerialPort >;

public:
    CanSerialPort();
    ~CanSerialPort();

    // !\brief Initialize CanSerialPort
    void Init();

    void SetDeviceType(uint8_t device_type) { m_DeviceType = device_type != 0; }
    uint8_t GetDeviceType() { return m_DeviceType; }

    void SetDevice(ICanDevice* device);

    // !\brief Set this module enabled
    void SetEnabled(bool enable);

    // !\brief Is this module enabled?
    bool IsEnabled();

    // !\brief Set serial port
    void SetComPort(uint16_t port);

    // !\brief Get serial port
    uint16_t GetComPort();

    // !\brief Add CAN frame to TX queue
    void AddToTxQueue(uint32_t frame_id, uint8_t data_len, uint8_t* data);

    // !\brief Add CAN frame to RX queue
    void AddToRxQueue(uint32_t frame_id, uint8_t data_len, uint8_t* data);

    void SendPendingCanFrames(CallbackAsyncSerial& serial_port);

private:
    // !\brief Stops worker thread
    void DestroyWorkerThread();

    // !\brief Worker thread
    void WorkerThread();

    // !\brief Called when data was received via serial port (called by boost::asio::read_some)
    // !\param serial_port [in] Pointer to received data
    // !\param len [in] Received data length
    void OnDataReceived(const char* data, unsigned int len);

    // !\brief Is serial port data receiving enabled?
    bool is_enabled = true;

    // !\brief COM port number
    uint16_t com_port = 5;

    // !\brief Worker thread
    std::unique_ptr<std::thread> m_worker = nullptr;

    // !\brief Exit working thread?
    std::atomic<bool> to_exit = false;

    // !\brief Conditional variable for main thread exiting
    std::condition_variable m_cv;

    // !\brief Mutex for main thread
    std::mutex m_mutex;

    // !\brief Mutex for received data processing
    std::mutex m_RxMutex;

    // !\brief Circular buffer for received data
    boost::circular_buffer<char> m_CircBuff;

    // !\brief CAN Tx Queue
    std::queue<std::shared_ptr<CanData>> m_TxQueue;

    // !\brief CAN Device
    ICanDevice* m_Device;

    // !\brief CAN Device type
    uint8_t m_DeviceType = 0;
};