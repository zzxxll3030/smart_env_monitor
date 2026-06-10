#include "app_menu.h"
#include "OLED.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* ============================ 全局状态 ============================ */

static MenuPage_t     g_page       = PAGE_HOME;
static EditState_t    g_edit       = EDIT_NONE;
static uint8_t        g_cursor     = 0;      /* 当前高亮项索引 */
static RuntimeThreshold_t g_thr;             /* 运行时阈值 */
static uint32_t       g_boot_ticks = 0;      /* 启动时刻 */

/* ============================ 首页选项标签 ============================ */

static const char *home_labels[] = {
    "1.Main Data",
    "2.Detail",
    "3.Threshold",
    "4.Sys Info"
};

/* ============================ 阈值名 ============================ */

static const char *thr_names[] = {
    "T_hi", "T_lo", "H_hi", "H_lo", "L_lo", "D_nr"
};

/* 各阈值的调节步长 */
static const float thr_step[] = { 0.5f, 0.5f, 1.0f, 1.0f, 5.0f, 1.0f };

/* ============================ 辅助函数 ============================ */

/* 从 RuntimeThreshold_t 按索引读取 float 值 */
static float thr_get(uint8_t idx)
{
    switch (idx) {
    case 0: return g_thr.temp_high;
    case 1: return g_thr.temp_low;
    case 2: return g_thr.humi_high;
    case 3: return g_thr.humi_low;
    case 4: return g_thr.light_low;
    case 5: return g_thr.dist_near;
    default: return 0;
    }
}

static void thr_set(uint8_t idx, float val)
{
    switch (idx) {
    case 0: g_thr.temp_high = val; break;
    case 1: g_thr.temp_low  = val; break;
    case 2: g_thr.humi_high = val; break;
    case 3: g_thr.humi_low  = val; break;
    case 4: g_thr.light_low = (uint8_t)val; break;
    case 5: g_thr.dist_near = val; break;
    }
}

/* ============================ 初始化 ============================ */

void Menu_Init(void)
{
    g_page   = PAGE_HOME;
    g_edit   = EDIT_NONE;
    g_cursor = 0;
    g_boot_ticks = osKernelGetTickCount();

    /* 加载默认阈值（仅 RAM，无 Flash） */
    g_thr.temp_high = DEFAULT_THR_TEMP_HIGH;
    g_thr.temp_low  = DEFAULT_THR_TEMP_LOW;
    g_thr.humi_high = DEFAULT_THR_HUMI_HIGH;
    g_thr.humi_low  = DEFAULT_THR_HUMI_LOW;
    g_thr.light_low = DEFAULT_THR_LIGHT_LOW;
    g_thr.dist_near = DEFAULT_THR_DIST_NEAR;
}

void Menu_SetPage(MenuPage_t page)
{
    g_page = page;
    g_edit = EDIT_NONE;
    g_cursor = 0;
}

MenuPage_t Menu_GetPage(void)  { return g_page; }

const RuntimeThreshold_t* Menu_GetThreshold(void) { return &g_thr; }

/* 外部设置阈值（通过 UART/云端控制） */
void Menu_SetThresholdByIndex(uint8_t idx, float val)
{
    thr_set(idx, val);
}

void Menu_SaveThresholds(void)
{
    /* 阈值暂存 RAM，断电丢失 */
    printf("[Menu] Thresholds updated (RAM only)\r\n");
}

int Menu_PropNameToIndex(const char *prop)
{
    if (!strcmp(prop, "maxtemp_set"))  return THR_TEMP_HIGH_IDX;
    if (!strcmp(prop, "minitemp_set")) return THR_TEMP_LOW_IDX;
    if (!strcmp(prop, "maxhum_set"))   return THR_HUMI_HIGH_IDX;
    if (!strcmp(prop, "minihum_set"))  return THR_HUMI_LOW_IDX;
    if (!strcmp(prop, "minlight_set")) return THR_LIGHT_LOW_IDX;
    if (!strcmp(prop, "neardist_set")) return THR_DIST_NEAR_IDX;
    return -1;
}

/* ============================ 输入处理 ============================ */

void Menu_ProcessKey1(ButtonEvent_t evt)
{
    if (evt != BTN_SHORT) return;

    if (g_edit != EDIT_NONE) return;    /* 编辑中不切页 */

    g_page   = (MenuPage_t)((g_page + 1) % PAGE_COUNT);
    g_cursor = 0;
}

void Menu_ProcessKey2(ButtonEvent_t evt)
{
    if (g_page != PAGE_THRESHOLD) {
        /* 非阈值页：长按回首页 */
        if (evt == BTN_LONG) {
            g_page   = PAGE_HOME;
            g_edit   = EDIT_NONE;
            g_cursor = 0;
        }
        /* 首页短按：进入选中页面 */
        if (g_page == PAGE_HOME && evt == BTN_SHORT) {
            /* cursor 0→MAIN, 1→DETAIL, 2→THRESHOLD, 3→INFO */
            g_page = (MenuPage_t)(g_cursor + 1);
            g_cursor = 0;
        }
        return;
    }

    /* ==== 阈值页 ==== */

    if (evt == BTN_LONG) {
        /* 长按：直接退出阈值页 */
        g_page = PAGE_HOME;
        g_edit = EDIT_NONE;
        g_cursor = 0;
        return;
    }

    /* 短按：状态机 */
    switch (g_edit) {

    case EDIT_NONE:
        /* 进入选择模式 */
        g_edit   = EDIT_SELECT;
        g_cursor = 0;
        break;

    case EDIT_SELECT:
        /* 选中当前项，进入数值编辑 */
        g_edit = EDIT_VALUE;
        break;

    case EDIT_VALUE:
        /* 确认数值，保存到 RAM 并同步云端 */
        Menu_SaveThresholds();
        {
            static char json[256];  /* static 不占任务栈，防止栈溢出 */
            snprintf(json, sizeof(json),
                "{\"cmd\":\"report\",\"thr\":{"
                "\"maxtemp_set\":%.1f,"
                "\"minitemp_set\":%.1f,"
                "\"maxhum_set\":%.0f,"
                "\"minihum_set\":%.0f,"
                "\"minlight_set\":%.0f,"
                "\"neardist_set\":%.1f"
                "}}\r\n",
                g_thr.temp_high, g_thr.temp_low,
                g_thr.humi_high, g_thr.humi_low,
                (float)g_thr.light_low, g_thr.dist_near);
            HAL_UART_Transmit(&huart1, (uint8_t *)json, strlen(json), 100);
        }
        g_edit = EDIT_SELECT;
        break;
    }
}

void Menu_ProcessEncoder(int32_t steps)
{
    if (steps == 0) return;

    if (g_page == PAGE_HOME) {
        /* 首页：切换高亮页选项 */
        int8_t c = (int8_t)g_cursor + ((steps > 0) ? 1 : -1);
        if (c < 0) c = 3;
        if (c > 3) c = 0;
        g_cursor = (uint8_t)c;
        return;
    }

    if (g_page == PAGE_THRESHOLD) {
        if (g_edit == EDIT_SELECT) {
            /* 选择模式：切换高亮阈值项 */
            int8_t c = (int8_t)g_cursor + ((steps > 0) ? 1 : -1);
            if (c < 0) c = THR_COUNT - 1;
            if (c >= THR_COUNT) c = 0;
            g_cursor = (uint8_t)c;
        } else if (g_edit == EDIT_VALUE) {
            /* 编辑模式：调节数值（仅改内存） */
            float val = thr_get(g_cursor);
            float step = thr_step[g_cursor];
            val += (float)steps * step;  /* 累加所有步数 */

            /* 边界限制 */
            if (val < -40.0f) val = -40.0f;
            if (val > 125.0f) val = 125.0f;
            if (g_cursor == THR_LIGHT_LOW_IDX) {
                if (val < 0)   val = 0;
                if (val > 100) val = 100;
            }
            if (g_cursor == THR_DIST_NEAR_IDX) {
                if (val < 1.0f)  val = 1.0f;
                if (val > 400.0f) val = 400.0f;
            }
            thr_set(g_cursor, val);
            /* 保存并上报，KEY2 确认时触发 */
        }
    }
}

/* ============================ 渲染 ============================ */

void Menu_Render(const SensorData_t *data)
{
    OLED_Clear();

    switch (g_page) {

    /* ---- 首页：页面选择 ---- */
    case PAGE_HOME:
        for (uint8_t i = 0; i < 4; i++) {
            char prefix = (i == g_cursor) ? '>' : ' ';
            OLED_Printf(0, i * 16, OLED_8X16, "%c%s", prefix, home_labels[i]);
        }
        break;

    /* ---- 主界面：传感器实时数据 ---- */
    case PAGE_MAIN:
        OLED_Printf(0, 0,  OLED_8X16, "T:%.1fC H:%.1f%%",
                    data->temperature, data->humidity);
        OLED_Printf(0, 16, OLED_8X16, "L:%d%%  D:%.1fcm",
                    data->light_percent, data->distance);
        OLED_Printf(0, 32, OLED_8X16, "Alarm:  OK    ");
        OLED_Printf(0, 48, OLED_8X16, "KEY1:next page");
        break;

    /* ---- 详情：统计数据 ---- */
    case PAGE_DETAIL:
        OLED_ShowString(0, 0, "--1min Stats--", OLED_8X16);
        OLED_Printf(0, 16, OLED_8X16, "T %.1f/%.1f/%.1f",
                    data->temperature, data->temperature, data->temperature);
        OLED_Printf(0, 32, OLED_8X16, "H %.1f/%.1f/%.1f",
                    data->humidity, data->humidity, data->humidity);
        OLED_Printf(0, 48, OLED_8X16, "L %d  D %.1f",
                    data->light_percent, data->distance);
        break;

    /* ---- 阈值编辑 ---- */
    case PAGE_THRESHOLD:
        if (g_edit == EDIT_VALUE) {
            /* 编辑模式：显示当前正在调节的项 */
            OLED_Printf(0, 0,  OLED_8X16, "-Edit %s-", thr_names[g_cursor]);
            OLED_Printf(8, 16, OLED_8X16, "[ %.1f ]", thr_get(g_cursor));
            OLED_ShowString(0, 32, "CW:+  CCW:-", OLED_8X16);
            OLED_ShowString(0, 48, "KEY2:Confirm", OLED_8X16);
        } else {
            /* 浏览/选择模式：滚动显示 4 项 */
            uint8_t start = 0;
            if (g_cursor > 2) start = g_cursor - 2;  /* 让选中行尽量在中间 */

            OLED_ShowString(0, 0, "Threshold", OLED_8X16);

            for (uint8_t i = 0; i < 3 && (start + i) < THR_COUNT; i++) {
                uint8_t idx = start + i;
                char prefix = (idx == g_cursor && g_edit == EDIT_SELECT) ? '>' : ' ';
                OLED_Printf(0, (i + 1) * 16, OLED_8X16, "%c%s:%.1f",
                            prefix, thr_names[idx], thr_get(idx));
            }
        }
        break;

    /* ---- 系统信息 ---- */
    case PAGE_INFO: {
        uint32_t now = osKernelGetTickCount();
        uint32_t sec = (now - g_boot_ticks) / 1000;
        uint32_t h = sec / 3600;
        uint32_t m = (sec % 3600) / 60;
        uint32_t s = sec % 60;

        OLED_ShowString(0, 0,  "-- System --", OLED_8X16);
        OLED_ShowString(0, 16, "FW:v1.0  72MHz", OLED_8X16);
        OLED_ShowString(0, 32, "Storage: RAM", OLED_8X16);
        OLED_Printf(0, 48, OLED_8X16, "Up:%02luh%02lum%02lus", h, m, s);
        break;
    }

    default: break;
    }

    OLED_Update();
}
