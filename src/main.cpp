#include "pch.h"
#include <iostream>
using namespace winrt;
using namespace winrt::Windows::Foundation;
using json = nlohmann::json;

namespace ns {
    struct Item
    {
        std::string title;
        std::string path;
        std::string args;
    };

    struct Config
    {
        std::string category;
        std::vector<Item> items;
    };

    void from_json(const json& j, Item& p)
    {
        j.at("title").get_to(p.title);
        j.at("path").get_to(p.path);
        j.at("args").get_to(p.args);
    }

    void from_json(const json& j, Config& p)
    {
        j.at("category").get_to(p.category);
        j.at("items").get_to(p.items);
    }
}

class Engine
{
public:
    Engine()
    {
        winrt::init_apartment();
    }
    ~Engine() { winrt::uninit_apartment(); }
    HRESULT CreateJumpList(const ns::Config& config)
    {
        auto customDestinationList = wil::CoCreateInstance<ICustomDestinationList, wil::err_returncode_policy>(CLSID_DestinationList);

        UINT maxSlots;
        winrt::com_ptr<IObjectArray> removedItems;
        RETURN_IF_FAILED(customDestinationList->BeginList(&maxSlots, IID_PPV_ARGS(removedItems.put())));

        // Create a custom category
        auto objectCollection = wil::CoCreateInstance<IObjectCollection, wil::err_returncode_policy>(CLSID_EnumerableObjectCollection);

        // Add tasks to the custom category
        for(auto& item: config.items)
        {
            AddTaskToCollection(objectCollection.get(),
                                string_to_w(item.title).c_str(),
                                string_to_w(item.path).c_str(),
                                string_to_w(item.args).c_str()
            );
        }

        auto objectArray = objectCollection.try_query<IObjectArray>();
        customDestinationList->AppendCategory(string_to_w(config.category).c_str(), objectArray.get());

        return customDestinationList->CommitList();
    }
private:
    HRESULT AddTaskToCollection(IObjectCollection *collection, LPCWSTR title, LPCWSTR appPath, LPCWSTR args)
    {
        // use wil::err_returncode_policy to return hresult fast.
        auto shellLink = wil::CoCreateInstance<IShellLink, wil::err_returncode_policy>(CLSID_ShellLink);
        RETURN_IF_FAILED(shellLink->SetPath(appPath));
        RETURN_IF_FAILED(shellLink->SetArguments(args));
        RETURN_IF_FAILED(shellLink->SetDescription(title));
        {
            auto propertyStore = shellLink.try_query<IPropertyStore>();
            wil::unique_prop_variant propVariant;
            RETURN_IF_FAILED(InitPropVariantFromString(title, propVariant.addressof()));
            RETURN_IF_FAILED(propertyStore->SetValue(PKEY_Title, propVariant));
            RETURN_IF_FAILED(propertyStore->Commit());
        }
        return collection->AddObject(shellLink.get());
    }
    std::wstring string_to_w(const std::string& str)
    {
        return std::wstring(str.begin(), str.end());
    }
};

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Engine eng;
    std::ifstream f("config.json");
    if (f.is_open())
    {
        json data = json::parse(f,nullptr,false);
        //MessageBox(nullptr,data.dump().c_str(),nullptr,MB_OK);
        //std::cout << data.dump();
        if(!data.is_null())
        {
            ns::Config config = data.get<ns::Config>();
            
            HRESULT hr = eng.CreateJumpList(config);
            if(S_OK == hr){
                MessageBox(nullptr,L"Successfully updated shortcuts",L"Shortcuts",MB_OK|MB_ICONINFORMATION);
            }
            else{
                MessageBox(nullptr,L"Failed updated shortcuts",L"Shortcuts",MB_OK|MB_ICONERROR);
             }
        }
    }
    return 0;
}