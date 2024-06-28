#include "pch.h"
using namespace winrt;
using namespace winrt::Windows::Foundation;
using json = nlohmann::json;

namespace ns
{
    struct Item
    {
        std::string title;
        std::string path;
        std::string args;
    };

    struct Config
    {
        std::vector<Item> items;
        u_short iconInTaskbar = 0;
    };

    void from_json(const json &j, Item &p)
    {
        j.at("title").get_to(p.title);
        j.at("path").get_to(p.path);
        j.at("args").get_to(p.args);
    }

    void from_json(const json &j, Config &p)
    {
        j.at("items").get_to(p.items);
        j.at("iconInTaskbar").get_to(p.iconInTaskbar);
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
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;
    Engine(Engine &&) = delete;
    Engine &operator=(Engine &&) = delete;

    static HRESULT CreateJumpList(const ns::Config &config)
    {
        auto customDestinationList = wil::CoCreateInstance<ICustomDestinationList, wil::err_returncode_policy>(CLSID_DestinationList);

        UINT maxSlots{};
        winrt::com_ptr<IObjectArray> removedItems;
        RETURN_IF_FAILED(customDestinationList->BeginList(&maxSlots, IID_PPV_ARGS(removedItems.put())));

        // Create a custom category
        auto objectCollection = wil::CoCreateInstance<IObjectCollection, wil::err_returncode_policy>(CLSID_EnumerableObjectCollection);

        // Add tasks to the custom category
        for (const auto &item : config.items)
        {
            AddTaskToCollection(objectCollection.get(),
                                string_to_w(item.title).c_str(),
                                string_to_w(item.path).c_str(),
                                string_to_w(item.args).c_str());
        }

        auto objectArray = objectCollection.try_query<IObjectArray>();
        HRESULT hr = customDestinationList->AddUserTasks(objectArray.get());
        customDestinationList->CommitList();
        return hr;
    }
    static void ShowMenu(u_short nth)
    {
        POINT pt = GetTaskBarPoint(nth);
        POINT ptCur{};

        GetCursorPos(&ptCur);
        ShowCursor(FALSE);
        SetCursorPos(pt.x, pt.y);

        //std::array<INPUT, 2> inputs{};
        INPUT inputs[2] = {}; //NOLINT
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; //NOLINT
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP; //NOLINT

        SendInput(2, inputs, sizeof(INPUT));        //NOLINT
        //SendInput(2, inputs.data(), inputs.size());
        SetCursorPos(ptCur.x, ptCur.y);
        ShowCursor(TRUE);
    }

    static bool ConfigChanged(const json &jsonConfig)
    {
        bool changed{true};
        size_t hash_in_file{0};
        std::ifstream iff{"quicklinks.hash"};

        std::hash<std::string> hasher;
        size_t hash_value = hasher(jsonConfig.at("items").dump());

        if (iff.is_open())
        {
            iff >> hash_in_file;
            iff.close();
            if (hash_value == hash_in_file)
            {
                changed = false;
            }
        }

        if (changed)
        {
            std::ofstream of{"quicklinks.hash"};
            of << hash_value;
            of.close();
        }
        return changed;
    }

private:
    // NOLINTNEXTLINE(readability-function-cognitive-complexity,bugprone-easily-swappable-parameters)
    static HRESULT AddTaskToCollection(IObjectCollection *collection, LPCWSTR title, LPCWSTR appPath, LPCWSTR args)
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
    static std::wstring string_to_w(const std::string &str)
    {
        return {str.begin(), str.end()};
    }
    // https://windhawk.net/mods/taskbar-icon-size
    // Icon size: 24x24, taskbar height: 48 (Windows 11 default)
    // width each Icon edge = 44, edge of first = 10
    static POINT GetTaskBarPoint(u_short nth)
    {
        POINT value{};
        APPBARDATA abd = {sizeof(APPBARDATA)};
        SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
        if (abd.uEdge == ABE_TOP || abd.uEdge == ABE_BOTTOM)
        {
            value = POINT{(abd.rc.left + (10 + 22 + 44 * nth)), (abd.rc.top + 24)}; // NOLINT
        }
        else
        {
            value = POINT{(abd.rc.left + 24), (abd.rc.top + (10 + 22 + 44 * nth))}; // NOLINT
        }
        return value;
    }
};

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Engine eng;
    std::ifstream f("config.json");
    if (f.is_open())
    {
        json data = json::parse(f, nullptr, false);
        if (!data.is_null())
        {
            ns::Config config = data.get<ns::Config>();
            if (!Engine::ConfigChanged(data))
            {
                // if no changed, show menu
                Engine::ShowMenu(config.iconInTaskbar);
            }
            else
            {
                // if config changed, register it in JumpList
                HRESULT hr = Engine::CreateJumpList(config);
                if (S_OK == hr)
                {
                    MessageBox(nullptr, L"Successfully updated shortcuts", L"Shortcuts", MB_OK | MB_ICONINFORMATION);
                }
                else
                {
                    MessageBox(nullptr, L"Failed updated shortcuts", L"Shortcuts", MB_OK | MB_ICONERROR);
                }
            }
        }
    }
    return 0;
}