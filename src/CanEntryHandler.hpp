#pragma once

#include "utils/CSingleton.hpp"
#include <filesystem>
#include <bitfield/bitfield.h>
#include <isotp/isotp.h>

constexpr uint8_t CAN_LOG_DIR_TX = 0;
constexpr uint8_t CAN_LOG_DIR_RX = 1;

constexpr size_t MAX_ISOTP_FRAME_LEN = 4096;

class CanEntryBase
{
public:
    CanEntryBase() = default;
    CanEntryBase(uint8_t* data_, uint8_t data_len)
    {
        if(data_ && data_len)
            data.insert(data.end(), data_, data_ + data_len);
    }

    CanEntryBase(const CanEntryBase& from) :
        data(from.data)
    {

    }
    std::vector<uint8_t> data{};
    std::chrono::steady_clock::time_point last_execution;
};

class CanEntryTransmitInfo
{
public:
    CanEntryTransmitInfo() = default;

    CanEntryTransmitInfo(const CanEntryTransmitInfo& from) :
        period(from.period), log_level(from.log_level)
    {

    }

    uint32_t period{};
    size_t count{};
    uint8_t log_level{};
};

class CanTxEntry : public CanEntryBase, public CanEntryTransmitInfo
{
public:
    CanTxEntry() = default;
    ~CanTxEntry() = default;
    CanTxEntry(const CanTxEntry& from) : 
        CanEntryBase(from), id(from.id + 1), CanEntryTransmitInfo(from), comment(from.comment)
    { 

    }
    uint32_t id{};
    std::string comment{};
    bool send = false;
    bool single_shot = false;
};

class CanRxData : public CanEntryBase, public CanEntryTransmitInfo
{
public:
    CanRxData(uint8_t* data_, uint8_t data_len) :
        CanEntryBase(data_, data_len)
    {
        count = 1;
    }
};

class CanLogEntry : public CanEntryBase
{
public:
    CanLogEntry(uint8_t dir, uint32_t frame_id_, uint8_t* data_, uint8_t data_len, std::chrono::steady_clock::time_point& timepoint) :
        CanEntryBase(data_, data_len)
    {
        frame_id = frame_id_ & 0x1FFFFFFF;
        direction = dir & 1;
        last_execution = timepoint;
    }
    union
    {
        uint32_t frame_id_and_direction;
        struct
        {
            uint32_t frame_id : 29;
            uint8_t direction : 1;  /* 0 = sent, 1 = received */
        };
    };
};

enum CanBitfieldType : uint8_t
{
    CBT_BOOL, CBT_UI8, CBT_I8, CBT_UI16, CBT_I16, CBT_UI32, CBT_I32, CBT_UI64, CBT_I64, CBT_FLOAT, CBT_DOUBLE, CBT_INVALID
};

class CanMap
{
public:
    CanMap(const std::string& name, CanBitfieldType type, uint8_t size, size_t min_val, size_t max_val) :
        m_Name(name), m_Type(type), m_Size(size), m_MinVal(min_val), m_MaxVal(max_val)
    {

    }

    //std::map<uint8_t, std::variant<bool, uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, float, double, std::string>> m_Type;

    // !\brief Mapping type
    CanBitfieldType m_Type;

    // !\brief Mapping name
    std::string m_Name;

    // !\brief Bit length (starting from it's offset)
    uint8_t m_Size;

    // !\brief Minimum value
    size_t m_MinVal;

    // !\brief Maximum value
    size_t m_MaxVal;
};

using CanMapping = std::map<uint32_t, std::map<uint8_t, std::unique_ptr<CanMap>>>;  /* [frame_id] = map[bit pos, size] */
using CanBitfieldInfo = std::vector<std::pair<std::string, std::string>>;

class ICanEntryLoader
{
public:
    virtual bool Load(const std::filesystem::path& path, std::vector<std::unique_ptr<CanTxEntry>>& e) = 0;
    virtual bool Save(const std::filesystem::path& path, std::vector<std::unique_ptr<CanTxEntry>>& e) = 0;
};

class XmlCanEntryLoader : public ICanEntryLoader
{
public:
    virtual ~XmlCanEntryLoader();

    bool Load(const std::filesystem::path& path, std::vector<std::unique_ptr<CanTxEntry>>& e) override;
    bool Save(const std::filesystem::path& path, std::vector<std::unique_ptr<CanTxEntry>>& e) override;
};

class ICanRxEntryLoader
{
public:
    virtual bool Load(const std::filesystem::path& path, std::unordered_map<uint32_t, std::string>& e, std::unordered_map<uint32_t, uint8_t>& loglevels) = 0;
    virtual bool Save(const std::filesystem::path& path, std::unordered_map<uint32_t, std::string>& e, std::unordered_map<uint32_t, uint8_t>& loglevels) = 0;
};

class XmlCanRxEntryLoader : public ICanRxEntryLoader
{
public:
    virtual ~XmlCanRxEntryLoader();

    bool Load(const std::filesystem::path& path, std::unordered_map<uint32_t, std::string>& e, std::unordered_map<uint32_t, uint8_t>& loglevels) override;
    bool Save(const std::filesystem::path& path, std::unordered_map<uint32_t, std::string>& e, std::unordered_map<uint32_t, uint8_t>& loglevels) override;
};

class ICanMappingLoader
{
public:
    virtual bool Load(const std::filesystem::path& path, CanMapping& mapping) = 0;
    virtual bool Save(const std::filesystem::path& path, CanMapping& mapping) = 0;
};

class XmlCanMappingLoader : public ICanMappingLoader
{
public:
    virtual ~XmlCanMappingLoader();

    bool Load(const std::filesystem::path& path, CanMapping& mapping) override;
    bool Save(const std::filesystem::path& path, CanMapping& mapping) override;

    static CanBitfieldType GetTypeFromString(const std::string_view& input);
    static const std::string_view GetStringFromType(CanBitfieldType type);

    static std::pair<int64_t, int64_t> GetMinMaxForType(CanBitfieldType type);

private:
    static inline std::map<CanBitfieldType, std::string> m_CanBitfieldTypeMap
    {
        {CBT_BOOL, "bool"},
        {CBT_UI8, "uint8_t"},
        {CBT_I8, "int8_t"},
        {CBT_UI16, "uint16_t"},
        {CBT_I16, "int16_t"},
        {CBT_UI32, "uint32_t"},
        {CBT_I32, "int32_t"},
        {CBT_UI64, "uint64_t"},
        {CBT_I64, "int64_t"},
        {CBT_FLOAT, "float"},
        {CBT_DOUBLE, "double"},
        {CBT_INVALID, "invalid"}
    };    
    
    static inline std::map<CanBitfieldType, std::pair<int64_t, int64_t>> m_CanTypeSizes
    {
        {CBT_BOOL, {0, 1}},
        {CBT_UI8, {std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max()}},
        {CBT_I8, {std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max()}},
        {CBT_UI16, {std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max()}},
        {CBT_I16, {std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max()}},
        {CBT_UI32, {std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max()}},
        {CBT_I32, {std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()}},
        //{CBT_UI64, "uint64_t"},
        {CBT_I64, {std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max()}},
        //{CBT_FLOAT, "float"},
        //{CBT_DOUBLE, "double"},
        {CBT_INVALID, {0, 0}}
    };
};

class CanEntryHandler
{
public:
    CanEntryHandler(ICanEntryLoader& loader, ICanRxEntryLoader& rx_loader, ICanMappingLoader& mapping_loader);
    ~CanEntryHandler();

    // !\brief Initialize entry handler
    void Init();

    // !\brief Load files (TX & RX List, Frame mapping)
    void LoadFiles();

    // !\brief Worker thread
    void WorkerThread(std::stop_token token);

    // !\brief Called when a can frame was sent
    void OnFrameSent(uint32_t frame_id, uint8_t data_len, uint8_t* data);

    // !\brief Called when a can frame was received
    void OnFrameReceived(uint32_t frame_id, uint8_t data_len, uint8_t* data);
    
    // !\brief Toggle automatic sending of all CAN frames which period isn't null
    // !\param toggle [in] Toggle auto send?
    void ToggleAutoSend(bool toggle);    
    
    // !\param Get CAN auto send state
    // !\return Is auto send toggled?
    bool IsAutoSend() { return auto_send; }

    // !\brief Toggle recording
    // !\param toggle [in] Toggle recording?
    // !\param is_puase [in] Is pause?
    void ToggleRecording(bool toggle, bool is_puase);

    // !\brief Clear recorded frames
    void ClearRecording();

    // !\brief Send Data Frame over CAN BUS
    // !\param frame_id [in] CAN Frame ID
    // !\param data [in] Data to send
    // !\param size [in] Data size
    void SendDataFrame(uint32_t frame_id, uint8_t* data, uint16_t size);

    // !\brief Send Iso-TP frame over CAN BUS
    // !\param data [in] Data to send
    // !\param size [in] Data size
    void SendIsoTpFrame(uint8_t* data, uint16_t size);

    // !\brief Load TX list from a file
    // !\param path [in] File path to load
    // !\return Is load was successfull?
    bool LoadTxList(std::filesystem::path& path);

    // !\brief Save TX list to a file
    // !\param path [in] File path to save
    bool SaveTxList(std::filesystem::path& path);

    // !\brief Load RX list from a file
    // !\param path [in] File path to load
    // !\return Is load was successfull?
    bool LoadRxList(std::filesystem::path& path);

    // !\brief Save RX list to a file
    // !\param path [in] File path to save
    bool SaveRxList(std::filesystem::path& path);
    
    // !\brief Load CAN mapping from a file
    // !\param path [in] File path to save
    bool LoadMapping(std::filesystem::path& path);

    // !\brief Save CAN mapping to a file
    // !\param path [in] File path to save
    bool SaveMapping(std::filesystem::path& path);

    // !\brief Save recorded data to file
    // !\param path [in] File path to save
    bool SaveRecordingToFile(std::filesystem::path& path);

    uint8_t GetRecordingLogLevel() { return m_RecodingLogLevel; }

    void SetRecordingLogLevel(uint8_t log_level) { m_RecodingLogLevel = log_level; }

    // !\brief Get log records for given frame
    // !\param frame_id [in] CAN Frame ID
    // !\param is_rx [in] Is RX?
    // !\param log [out] Vector of rows
    void GenerateLogForFrame(uint32_t frame_id, bool is_rx, std::vector<std::string>& log);

    // !\brief Return TX Frame count
    // !\return TX Frame count
    uint64_t GetTxFrameCount() { return tx_frame_cnt; }

    // !\brief Return RX Frame count
    // !\return RX Frame count
    uint64_t GetRxFrameCount() { return rx_frame_cnt; }

    // !\brief Get map for frame id (in string format)
    CanBitfieldInfo GetMapForFrameId(uint32_t frame_id, bool is_rx);

    void ApplyEditingOnFrameId(uint32_t frame_id, std::vector<std::string> new_data);

    // !\brief Vector of CAN TX entries
    std::vector<std::unique_ptr<CanTxEntry>> entries;

    // !\brief Vector of CAN RX entries
    std::unordered_map<uint32_t, std::unique_ptr<CanRxData>> m_rxData;  /* [frame_id] = rxData */

    // !\brief Frame ID comment
    std::unordered_map<uint32_t, std::string> rx_entry_comment;  /* [frame_id] = comment msg */

    // !\brief Frame ID log levels
    std::unordered_map<uint32_t, uint8_t> m_RxLogLevels;

    // !\brief CAN Log entries (both TX & RX)
    std::vector<std::unique_ptr<CanLogEntry>> m_LogEntries;

    // !\brief Path to default TX list
    std::filesystem::path default_tx_list;
    
    // !\brief Path to default RX list
    std::filesystem::path default_rx_list;    
    
    // !\brief Path to default CAN mapping
    std::filesystem::path default_mapping;

    // !\brief Mutex for entry handler
    std::mutex m;

private:
    // !\brief Assigns new TX buffer to TX entry
    void AssignNewBufferToTxEntry(uint32_t frame_id, uint8_t* buffer, size_t size);

    // !\brief Handle bit reading of a frame
    template <typename T> void HandleBitReading(uint32_t frame_id, bool is_rx, std::unique_ptr<CanMap>& m, size_t offset, CanBitfieldInfo& info);

    template <typename T> void HandleBitWriting(uint32_t frame_id, uint8_t& pos, uint8_t offset, uint8_t size, uint8_t* byte_array, std::vector<std::string>& new_data);

    // !\brief Find CAN TX Entry by Frame ID
    std::optional<std::reference_wrapper<CanTxEntry>> FindTxCanEntryByFrame(uint32_t frame_id);

    // !\brief Reference to CAN TX entry loader
    ICanEntryLoader& m_CanEntryLoader;

    // !\brief Reference to CAN RX entry loader
    ICanRxEntryLoader& m_CanRxEntryLoader;    
    
    // !\brief Reference to CAN mapping loader
    ICanMappingLoader& m_CanMappingLoader;

    // !\brief Sending every can frame automatically at startup which period is not null? 
    bool auto_send = false;

    // !\brief Is recording on?
    bool is_recoding = false;

    // !\brief TX Frame count
    uint64_t tx_frame_cnt = 0;

    // !\brief RX Frame count
    uint64_t rx_frame_cnt = 0;

    // !\brief Exit worker thread?
    //std::stop_source stop_source;

    // !\brief Worker thread
    std::unique_ptr<std::jthread> m_worker;

    // !\brief Starting time
    std::chrono::steady_clock::time_point start_time;

    // !\brief CAN mapping container
    CanMapping m_mapping;

    // !\brief Recording log level
    uint8_t m_RecodingLogLevel = 1;

    // !\brief ISO-TP Link
    IsoTpLink link;

    // !\brief ISO-TP Receiving buffer
    uint8_t m_Isotp_Sendbuf[MAX_ISOTP_FRAME_LEN];

    // !\brief ISO-TP Transmit buffer
    uint8_t m_Isotp_Recvbuf[MAX_ISOTP_FRAME_LEN];
};