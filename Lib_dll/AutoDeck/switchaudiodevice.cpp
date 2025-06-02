#include "pch.h"
#include <Windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <comdef.h>

// Definisi interface IPolicyConfig dan CLSID tidak resmi
interface IPolicyConfig : public IUnknown{
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(PCWSTR, void**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR, INT, void**) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(PCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR, void*, void*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR, INT, PINT64, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR, void*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR, void*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR wszDeviceId, ERole eRole) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR, INT) = 0;
};

// CLSID dan IID berdasarkan reverse engineering (seperti yang digunakan oleh NirCmd dan SoundVolumeView)
const GUID CLSID_PolicyConfigClient = { 0x870af99c, 0x171d, 0x4f9e, {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9} };
const GUID IID_IPolicyConfig = { 0xf8679f50, 0x850a, 0x41cf, {0x9c, 0x72, 0x43, 0x0f, 0x29, 0x02, 0x90, 0xc8} };

// Fungsi utama
extern "C" __declspec(dllexport)
bool __stdcall SwitchSoundDevice(const wchar_t* deviceName, int role = 0) {
    if (!deviceName) return false;

    HRESULT hrCoInit = CoInitialize(NULL);
    bool comInitialized = SUCCEEDED(hrCoInit);


    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDeviceCollection* pCollection = nullptr;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) return false;

    hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr)) {
        pEnumerator->Release();
        return false;
    }

    UINT count;
    pCollection->GetCount(&count);

    IMMDevice* pDevice = nullptr;
    bool found = false;
    for (UINT i = 0; i < count; i++) {
        IMMDevice* pItem;
        if (SUCCEEDED(pCollection->Item(i, &pItem))) {
            IPropertyStore* pProps;
            if (SUCCEEDED(pItem->OpenPropertyStore(STGM_READ, &pProps))) {
                PROPVARIANT varName;
                PropVariantInit(&varName);
                if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
                    if (wcsstr(varName.pwszVal, deviceName) != nullptr) {
                        pDevice = pItem;
                        pDevice->AddRef();
                        found = true;
                    }
                    PropVariantClear(&varName);
                }
                pProps->Release();
            }
            pItem->Release();
            if (found) break;
        }
    }

    bool result = false;

    if (found) {
        LPWSTR deviceId = nullptr;
        if (SUCCEEDED(pDevice->GetId(&deviceId))) {
            IPolicyConfig* pPolicyConfig = nullptr;
            hr = CoCreateInstance(CLSID_PolicyConfigClient, NULL, CLSCTX_ALL,
                IID_IPolicyConfig, (void**)&pPolicyConfig);
            if (SUCCEEDED(hr)) {
                hr = pPolicyConfig->SetDefaultEndpoint(deviceId, (ERole)role);
                result = SUCCEEDED(hr);
                pPolicyConfig->Release();
            }
            CoTaskMemFree(deviceId);
        }
        pDevice->Release();
    }

    pCollection->Release();
    pEnumerator->Release();
    if (comInitialized) CoUninitialize();


    return result;
}
