#pragma once
#include <windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <iostream>
#include <cstdint>

#define IOCTL_READ              CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_WRITE             CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_CR3           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_PROCESS_BASE  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x812, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_PEB           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x813, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

struct ReadRequest  { uint32_t pid; uint32_t padding; uint64_t address; void* buffer; size_t size; uint64_t cr3; };
struct WriteRequest { uint32_t pid; uint32_t padding; uint64_t address; void* buffer; size_t size; uint64_t cr3; };
struct Cr3Request   { uint32_t pid; uint64_t cr3_attach; uint64_t cr3_brute; };
struct ProcessBaseRequest { uint32_t pid; uint32_t padding; uint64_t output_address; };
struct PebRequest   { uint32_t pid; uint32_t padding; uint64_t peb; };

typedef NTSTATUS(NTAPI* pNtQueryInformationThread)(
    HANDLE ThreadHandle,
    THREADINFOCLASS ThreadInformationClass,
    PVOID ThreadInformation,
    ULONG ThreadInformationLength,
    PULONG ReturnLength
);

typedef struct _THREAD_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PVOID TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG_PTR AffinityMask;
    LONG Priority;
    LONG BasePriority;
} THREAD_BASIC_INFORMATION, * PTHREAD_BASIC_INFORMATION;

class c_driver {
private:
    HANDLE m_handle = INVALID_HANDLE_VALUE;
    uint32_t m_pid = 0;
    uint64_t m_base = 0;
    uint64_t m_cr3 = 0;

    uint32_t get_process_id(const wchar_t* process_name) {
        uint32_t pid = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32W);
            if (Process32FirstW(snapshot, &pe32)) {
                do {
                    if (wcscmp(pe32.szExeFile, process_name) == 0) {
                        pid = pe32.th32ProcessID;
                        break;
                    }
                } while (Process32NextW(snapshot, &pe32));
            }
            CloseHandle(snapshot);
        }
        return pid;
    }

public:
    c_driver() = default;
    ~c_driver() { if (m_handle != INVALID_HANDLE_VALUE) CloseHandle(m_handle); }

    bool connect() {
        m_handle = CreateFileW(L"\\\\.\\WinNotify", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (m_handle == INVALID_HANDLE_VALUE) {
            std::cout << "[!] Failed to connect to \\\\.\\WinNotify driver." << std::endl;
            return false;
        }
        std::cout << "[+] Connected to kernel driver." << std::endl;
        return true;
    }

    bool attach(const wchar_t* process_name) {
        m_pid = get_process_id(process_name);
        if (!m_pid) return false;

        Cr3Request cr3_req = { m_pid, 0, 0 };
        DWORD bytes;
        if (DeviceIoControl(m_handle, IOCTL_GET_CR3, &cr3_req, sizeof(cr3_req), &cr3_req, sizeof(cr3_req), &bytes, nullptr)) {
            m_cr3 = cr3_req.cr3_attach;
        }

        ProcessBaseRequest pb_req = { m_pid, 0, 0 };
        if (DeviceIoControl(m_handle, IOCTL_GET_PROCESS_BASE, &pb_req, sizeof(pb_req), &pb_req, sizeof(pb_req), &bytes, nullptr)) {
            m_base = pb_req.output_address;
        }

        std::cout << "[+] Attached to " << m_pid << " Base: 0x" << std::hex << m_base << " CR3: 0x" << m_cr3 << std::dec << std::endl;
        return (m_base != 0 && m_cr3 != 0);
    }

    bool read_raw(uint64_t address, void* buffer, size_t size) {
        if (m_handle == INVALID_HANDLE_VALUE) return false;
        ReadRequest req = { m_pid, 0, address, buffer, size, m_cr3 };
        DWORD bytes;
        return DeviceIoControl(m_handle, IOCTL_READ, &req, sizeof(req), &req, sizeof(req), &bytes, nullptr);
    }

    bool write_raw(uint64_t address, void* buffer, size_t size) {
        if (m_handle == INVALID_HANDLE_VALUE) return false;
        WriteRequest req = { m_pid, 0, address, buffer, size, m_cr3 };
        DWORD bytes;
        return DeviceIoControl(m_handle, IOCTL_WRITE, &req, sizeof(req), &req, sizeof(req), &bytes, nullptr);
    }

    template<typename T>
    T read(uint64_t address) {
        T buffer{};
        read_raw(address, &buffer, sizeof(T));
        return buffer;
    }

    template<typename T>
    bool write(uint64_t address, const T& value) {
        return write_raw(address, (void*)&value, sizeof(T));
    }

    uint64_t get_thread_teb(DWORD tid) {
        HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, tid);
        if (!hThread) return 0;

        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        if (!hNtdll) { CloseHandle(hThread); return 0; }

        auto pNtQIT = (pNtQueryInformationThread)GetProcAddress(hNtdll, "NtQueryInformationThread");
        if (!pNtQIT) { CloseHandle(hThread); return 0; }

        THREAD_BASIC_INFORMATION tbi;
        ULONG returnLength;
        NTSTATUS status = pNtQIT(hThread, (THREADINFOCLASS)0 /* ThreadBasicInformation */, &tbi, sizeof(tbi), &returnLength);
        CloseHandle(hThread);

        if (NT_SUCCESS(status)) {
            return (uint64_t)tbi.TebBaseAddress;
        }
        return 0;
    }

    uint32_t pid() const { return m_pid; }
    uint64_t base() const { return m_base; }
    uint64_t cr3() const { return m_cr3; }
};
