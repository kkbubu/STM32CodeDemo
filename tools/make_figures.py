"""生成可缩放中文接线、气路与步态示意图，坐标不代表真实尺寸。"""
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "docs"
INK, BLUE, GREEN, AMBER = "#183449", "#3278A4", "#268878", "#B87827"


class Figure:
    """为静态工程图提供少量 SVG 图元。"""

    def __init__(self, width, height, title, subtitle):
        self.parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
                      '<defs><marker id="arrow" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto"><path d="M0 0 L8 4 L0 8" fill="#3278A4"/></marker></defs>',
                      f'<rect width="100%" height="100%" fill="#F5F7F9"/><g font-family="Microsoft YaHei, SimHei, sans-serif" fill="{INK}">']
        self.text(36, 47, title, 28, weight="bold")
        self.text(36, 80, subtitle, 16, fill="#526B7A")

    def text(self, x, y, content, size=16, fill=INK, weight="normal", anchor="start"):
        self.parts.append(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" font-weight="{weight}" text-anchor="{anchor}">{escape(content)}</text>')

    def rect(self, x, y, width, height, fill="white", stroke="#C8D4DC", radius=12):
        self.parts.append(f'<rect x="{x}" y="{y}" width="{width}" height="{height}" rx="{radius}" fill="{fill}" stroke="{stroke}" stroke-width="1.5"/>')

    def line(self, x1, y1, x2, y2, color=BLUE, arrow=False, width=3):
        marker = ' marker-end="url(#arrow)"' if arrow else ""
        self.parts.append(f'<path d="M{x1} {y1} L{x2} {y2}" stroke="{color}" stroke-width="{width}" fill="none"{marker}/>')

    def save(self, name):
        (ROOT / name).write_text("\n".join(self.parts + ["</g></svg>"]), encoding="utf-8")


def wiring():
    """所有阀单端口映射及单通道 MOS 接线。"""
    fig = Figure(1320, 1130, "电气接线 · STM32F103C8T6 / HAL", "7 路当前配置 · 11 路预留 · 默认高电平 MOS · 线圈使用独立额定电源")
    fig.rect(36, 112, 270, 584, "#E9F1F6")
    fig.text(60, 150, "STM32F103C8T6", 22, weight="bold")
    fig.text(60, 180, "64 MHz HSI PLL", 16)
    labels = [("PB0", "CH1", "后端向外锚定"), ("PB1", "CH2", "后端向内抓取"),
              ("PB5", "CH3", "后伸缩 / 第 1 腔"), ("PB6", "CH4", "中间锚定"),
              ("PB7", "CH5", "前伸缩 / 第 1 腔"), ("PB8", "CH6", "前端向外锚定"),
              ("PB9", "CH7", "前端向内抓取"), ("PB10", "CH8", "后伸缩 / 第 2 腔"),
              ("PB11", "CH9", "后伸缩 / 第 3 腔"), ("PB12", "CH10", "前伸缩 / 第 2 腔"),
              ("PB13", "CH11", "前伸缩 / 第 3 腔")]
    for index, (pin, channel, role) in enumerate(labels):
        y = 223 + index * 40
        color = BLUE if index < 7 else AMBER
        fig.text(236, y + 6, pin, 17, fill=color, anchor="end")
        fig.line(250, y, 346, y, color, True)
        fig.rect(350, y - 15, 104, 30, "white", color, 5)
        fig.text(402, y + 6, f"MOS {channel}", 14, anchor="middle")
        fig.line(456, y, 493, y, color, True)
        fig.text(510, y + 6, role, 17, fill=color)
    fig.rect(782, 112, 502, 336)
    fig.text(806, 150, "控制与下载", 21, weight="bold")
    for index, line in enumerate(["PA9 TX → USB-TTL RX / PA10 RX ← TX", "USB-TTL：3.3 V 电平，共地，115200 8N1",
                                  "PA8 → 许可开关 → GND（闭合允许运行）", "PB14 → 常闭停止回路 → GND",
                                  "断线或按下：冻结输出，不自动泄压", "PA13 / PA14 → ST-Link SWDIO / SWCLK", "BOOT0 下拉；PC13：状态 LED"]):
        fig.text(806, 187 + index * 34, line, 16)
    fig.rect(782, 466, 502, 230)
    fig.text(806, 504, "可选反馈 · 当前关闭", 21, weight="bold")
    for index, line in enumerate(["PA0 / PA1 / PA2：后 / 中 / 前锚定就绪", "PA3：公共负压就绪", "现有接口接外部处理后的低有效数字信号", "模拟传感器须另做 ADC、标定与故障诊断", "压力就绪不等于机械抓持力已确认"]):
        fig.text(806, 539 + index * 29, line, 16)
    fig.rect(36, 720, 1248, 283)
    fig.text(60, 759, "单通道裸 N-MOS 低侧驱动示意（11 路相同）", 21, weight="bold")
    fig.text(68, 832, "GPIO", 18)
    fig.line(132, 825, 177, 825)
    fig.rect(178, 813, 75, 24, "#EEF3F7", BLUE, 1)
    fig.text(190, 803, "栅极电阻", 14)
    fig.line(254, 825, 317, 825)
    fig.line(290, 825, 290, 901)
    fig.rect(280, 857, 20, 38, "white", BLUE, 0)
    fig.text(208, 879, "10 kΩ", 14)
    fig.line(290, 901, 290, 943)
    fig.line(317, 800, 317, 854, INK)
    fig.line(327, 808, 327, 847, INK)
    fig.line(327, 808, 368, 808, INK)
    fig.line(368, 808, 368, 787, INK)
    fig.line(327, 847, 368, 847, INK)
    fig.line(368, 847, 368, 943, INK)
    fig.text(341, 836, "MOS", 15)
    fig.text(302, 790, "G", 14)
    fig.text(376, 808, "D", 14)
    fig.text(376, 874, "S", 14)
    fig.line(368, 787, 479, 787, INK)
    fig.rect(480, 769, 118, 38, "#DCECE7", GREEN, 4)
    fig.text(499, 794, "阀线圈", 17)
    fig.line(598, 787, 653, 787, INK)
    fig.text(668, 794, "+V 阀电源", 18)
    fig.line(445, 787, 445, 850, INK)
    fig.line(624, 787, 624, 850, INK)
    fig.line(445, 850, 515, 850, INK)
    fig.line(556, 850, 624, 850, INK)
    fig.parts.append('<path d="M515 835 L515 865 L544 850 Z" fill="none" stroke="#183449" stroke-width="2"/><path d="M550 833 L550 867" stroke="#183449" stroke-width="3"/>')
    fig.text(454, 890, "续流二极管：阴极接 +V", 16)
    fig.line(110, 943, 710, 943, INK)
    fig.text(110, 977, "公共 GND：STM32、MOS S、阀电源负极", 17)
    for index, line in enumerate(["• 栅极在 3.3 V 下可靠导通", "• 线圈电源电压与额定电压一致", "• 有驱动模块时核对内置续流与极性", "• MCU 复位期间依靠外部电阻关断", "• 线圈需允许持续通电"]):
        fig.text(806, 807 + index * 36, line, 17)
    fig.rect(36, 1024, 1248, 69, "#FFF0D9", "#D6AC65")
    fig.text(60, 1065, "掉电 / 复位会使阀泄压；必须用独立防坠措施。软件 STOP 保持电平，不能保证机械立即停止。", 18, fill="#895715")
    fig.save("wiring.svg")


def pneumatics():
    """模块拓扑和真空阀逻辑，不使用未经确认的厂商端口编号。"""
    fig = Figure(1320, 670, "机器人气路 · 外撑与内抓互斥", "沿管道由后向前排列；竖直安装前端朝上时 FWD 为上行，REV 为下行")
    fig.line(85, 184, 1235, 184, "#B8C8D3", width=8)
    fig.line(85, 347, 1235, 347, "#B8C8D3", width=8)
    modules = [(90, 170, "后端双向抓手", "CH1 外撑", "CH2 内抓", BLUE),
               (307, 170, "后伸缩段", "CH3 共用气路", "11 路：CH3/8/9", AMBER),
               (524, 170, "中间锚定", "CH4 负压锚定", "泄压松开", GREEN),
               (741, 170, "前伸缩段", "CH5 共用气路", "11 路：CH5/10/11", AMBER),
               (958, 230, "前端双向抓手", "CH6 外撑", "CH7 内抓", BLUE)]
    for x, width, title, first, second, color in modules:
        fig.rect(x, 208, width, 115, "white", color)
        fig.text(x + width / 2, 240, title, 19, fill=color, weight="bold", anchor="middle")
        fig.text(x + width / 2, 272, first, 17, anchor="middle")
        fig.text(x + width / 2, 301, second, 15, anchor="middle")
    fig.line(940, 143, 1174, 143, BLUE, True)
    fig.text(947, 130, "FWD 方向 / 前端朝上", 16)
    fig.rect(36, 390, 594, 226)
    fig.text(60, 429, "每一路二位三通阀", 21, weight="bold")
    fig.rect(66, 454, 236, 67, "#DCECE7", GREEN)
    fig.text(184, 482, "通电：腔体接负压", 18, anchor="middle")
    fig.text(184, 506, "气流：腔体 → 阀 → 真空源", 15, anchor="middle")
    fig.rect(330, 454, 270, 67, "#E9F1F6", BLUE)
    fig.text(465, 482, "断电：腔体通大气", 18, anchor="middle")
    fig.text(465, 506, "大气口 → 阀 → 腔体", 15, anchor="middle")
    fig.text(60, 556, "真空源接公共歧管；各阀工作口接对应模块。", 17)
    fig.text(60, 584, "实际口号按所用阀手册确认；保留大气泄放路径。", 17)
    fig.rect(650, 390, 634, 226)
    fig.text(674, 429, "机械动作约束", 21, weight="bold")
    for index, line in enumerate(["外撑 / 内抓：同一端不得同时通负压。", "伸缩段：负压缩短，通大气后依赖结构回弹伸长。", "中锚平移：前段缩短与后段伸长需要兼容行程。", "本图表示气路逻辑，不代表尺寸、阀口号或传感器布置。"]):
        fig.text(674, 468 + index * 36, line, 17)
    fig.save("pneumatics.svg")


def gait():
    """稳态六步图，以锚点颜色和变长模块说明位移。"""
    fig = Figure(1320, 1040, "垂直爬行 · 锚点交接步态", "蓝色锚点为通负压外撑；灰色为释放。图中前端在右，实际竖直安装时前端朝上。")
    steps = [("1 释放前端 → 前段伸长", (1, 1, 0), (0, 1), "后端 + 中锚承载；前段泄压回弹"),
             ("2 建立前端锚定", (1, 1, 1), (0, 1), "保留后端与中锚，等待前端建立支撑"),
             ("3 释放中锚 → 中锚前移", (1, 0, 1), (1, 0), "前段吸气缩短，同时后段泄压伸长"),
             ("4 建立中间锚定", (1, 1, 1), (1, 0), "保留前后两端，等待中间建立支撑"),
             ("5 释放后端 → 后段缩短", (0, 1, 1), (0, 0), "中锚 + 前端承载；后段吸气收拢后端"),
             ("6 建立后端锚定 → 循环", (1, 1, 1), (0, 0), "三处外撑，两段缩短；下一周期从步骤 1 开始")]
    for index, (title, anchors, lengths, note) in enumerate(steps):
        y = 115 + index * 132
        fig.rect(36, y, 1248, 116)
        fig.text(58, y + 37, title, 20, weight="bold")
        fig.text(58, y + 76, note, 15)
        # 步骤 5/6 向前移动后端，保持已经锚定的中端和前端位置。
        x = 751 if index >= 4 else 697
        for anchor in range(3):
            fill = BLUE if anchors[anchor] else "#DFE5E9"
            fig.rect(x, y + 31, 58, 50, fill, fill, 8)
            fig.text(x + 29, y + 63, ("后", "中", "前")[anchor], 20, "white" if anchors[anchor] else "#71838E", anchor="middle")
            x += 58
            if anchor < 2:
                length = 145 if lengths[anchor] else 91
                points = [(x, y + 56)]
                for j in range(1, 10):
                    points.append((x + j * length / 10, y + (40 if j % 2 else 72)))
                points.append((x + length, y + 56))
                fig.parts.append('<polyline points="' + " ".join(f"{a},{b}" for a, b in points) + '" fill="none" stroke="#B87827" stroke-width="4"/>')
                x += length
    fig.rect(36, 920, 1248, 80, "#FFF0D9", "#D6AC65")
    fig.text(58, 952, "首次启动另含准备：三锚点建立 → 后段收缩准备 → 前段收缩准备；REV 按前后镜像执行。", 17)
    fig.text(58, 979, "默认按时间交接，不代表真实抓牢；须验证回弹、承重与有效行程，并使用独立承重防坠绳。", 17)
    fig.save("gait.svg")


if __name__ == "__main__":
    wiring()
    pneumatics()
    gait()
    print("已生成 wiring.svg、pneumatics.svg、gait.svg")
