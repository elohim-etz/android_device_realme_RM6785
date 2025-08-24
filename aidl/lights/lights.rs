/*
 * Copyright (C) The Android Open Source Project
 * Copyright (C) The LineageOS Project
 * Copyright (C) Yet Another AOSP Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

use std::fs;
use std::io::Write;
use std::path::Path;

use log::{debug, warn};

use android_hardware_light::aidl::android::hardware::light::{
    HwLight::HwLight, HwLightState::HwLightState, ILights::ILights, LightType::LightType,
};

use binder::{ExceptionCode, Interface, Status};

static BRIGHTNESS_TABLE_0X7FF: [u32; 256] = [
    0,    1,    9,    33,   49,   73,   89,   113,  218,  299,  379,  444,
    484,  540,  580,  629,  669,  685,  725,  766,  790,  830,  854,  870,
    911,  935,  951,  991,  1015, 1040, 1056, 1080, 1096, 1120, 1136, 1160,
    1160, 1160, 1160, 1225, 1225, 1241, 1265, 1265, 1281, 1281, 1305, 1305,
    1322, 1322, 1346, 1346, 1362, 1362, 1386, 1386, 1402, 1402, 1426, 1426,
    1426, 1450, 1450, 1450, 1467, 1467, 1467, 1491, 1491, 1491, 1507, 1507,
    1507, 1531, 1531, 1531, 1531, 1547, 1547, 1547, 1547, 1571, 1571, 1587,
    1587, 1587, 1587, 1587, 1612, 1612, 1612, 1636, 1636, 1636, 1636, 1636,
    1652, 1652, 1652, 1652, 1652, 1676, 1676, 1676, 1676, 1676, 1692, 1692,
    1692, 1692, 1692, 1716, 1716, 1716, 1716, 1716, 1732, 1732, 1732, 1732,
    1732, 1732, 1732, 1732, 1757, 1757, 1757, 1757, 1773, 1773, 1773, 1773,
    1773, 1773, 1797, 1797, 1797, 1797, 1797, 1797, 1797, 1797, 1797, 1797,
    1813, 1813, 1813, 1813, 1813, 1813, 1813, 1837, 1837, 1837, 1837, 1837,
    1837, 1837, 1861, 1861, 1861, 1861, 1861, 1861, 1861, 1861, 1861, 1861,
    1877, 1877, 1877, 1877, 1877, 1877, 1877, 1877, 1877, 1902, 1902, 1902,
    1902, 1902, 1902, 1902, 1902, 1902, 1918, 1918, 1918, 1918, 1918, 1918,
    1918, 1918, 1918, 1918, 1942, 1942, 1942, 1942, 1942, 1942, 1942, 1942,
    1942, 1942, 1958, 1958, 1958, 1958, 1958, 1958, 1958, 1958, 1958, 1958,
    1958, 1958, 1982, 1982, 1982, 1982, 1982, 1982, 1982, 1982, 1982, 1982,
    1982, 1998, 1998, 1998, 1998, 1998, 1998, 1998, 1998, 1998, 1998, 1998,
    1998, 2022, 2022, 2022, 2022, 2022, 2022, 2022, 2022, 2022, 2022, 2022,
    2022, 2022, 2047, 2047,
];

static BRIGHTNESS_TABLE_0XFFF: [u32; 256] = [
    0,    1,    17,   65,   97,   146,  178,  226,  436,  597,  758,  887,
    968,  1080, 1161, 1258, 1338, 1371, 1451, 1532, 1580, 1661, 1709, 1741,
    1822, 1870, 1902, 1983, 2031, 2080, 2112, 2160, 2193, 2241, 2273, 2322,
    2322, 2322, 2322, 2450, 2450, 2483, 2531, 2531, 2563, 2563, 2612, 2612,
    2644, 2644, 2692, 2692, 2724, 2724, 2773, 2773, 2805, 2805, 2853, 2853,
    2853, 2902, 2902, 2902, 2934, 2934, 2934, 2982, 2982, 2982, 3015, 3015,
    3015, 3063, 3063, 3063, 3063, 3095, 3095, 3095, 3095, 3144, 3144, 3176,
    3176, 3176, 3176, 3176, 3224, 3224, 3224, 3272, 3272, 3272, 3272, 3272,
    3305, 3305, 3305, 3305, 3305, 3353, 3353, 3353, 3353, 3353, 3385, 3385,
    3385, 3385, 3385, 3434, 3434, 3434, 3434, 3434, 3466, 3466, 3466, 3466,
    3466, 3466, 3466, 3466, 3514, 3514, 3514, 3514, 3546, 3546, 3546, 3546,
    3546, 3546, 3595, 3595, 3595, 3595, 3595, 3595, 3595, 3595, 3595, 3595,
    3627, 3627, 3627, 3627, 3627, 3627, 3627, 3675, 3675, 3675, 3675, 3675,
    3675, 3675, 3724, 3724, 3724, 3724, 3724, 3724, 3724, 3724, 3724, 3724,
    3756, 3756, 3756, 3756, 3756, 3756, 3756, 3756, 3756, 3804, 3804, 3804,
    3804, 3804, 3804, 3804, 3804, 3804, 3837, 3837, 3837, 3837, 3837, 3837,
    3837, 3837, 3837, 3837, 3885, 3885, 3885, 3885, 3885, 3885, 3885, 3885,
    3885, 3885, 3917, 3917, 3917, 3917, 3917, 3917, 3917, 3917, 3917, 3917,
    3917, 3917, 3966, 3966, 3966, 3966, 3966, 3966, 3966, 3966, 3966, 3966,
    3966, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998,
    3998, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046,
    4046, 4046, 4095, 4095,
];

// Keep sorted in the order of importance.
const BACKENDS: &[LightType] = &[LightType::BACKLIGHT];

const LCD_LED_DIR: &str = "/sys/class/leds/lcd-backlight/";
const BRIGHTNESS_FILE: &str = "brightness";
const MAX_BRIGHTNESS_FILE: &str = "max_brightness";

#[derive(Default)]
pub struct LightsService;

impl Interface for LightsService {}

impl LightsService {
    fn get_brightness_from_state(state: &HwLightState) -> u32 {
        // Extract brightness from AARRGGBB
        let alpha = ((state.color >> 24) & 0xFF) as u32;
        let red = ((state.color >> 16) & 0xFF) as u32;
        let green = ((state.color >> 8) & 0xFF) as u32;
        let blue = (state.color & 0xFF) as u32;

        // Scale RGB brightness using Alpha brightness.
        let red = red * alpha / 0xFF;
        let green = green * alpha / 0xFF;
        let blue = blue * alpha / 0xFF;

        (77 * red + 150 * green + 29 * blue) >> 8
    }

    fn scale_brightness(brightness: u32, max_brightness: u32) -> u32 {
        debug!("Received brightness: {}", brightness);

        match max_brightness {
            4095 => BRIGHTNESS_TABLE_0XFFF[brightness as usize],
            2047 => BRIGHTNESS_TABLE_0X7FF[brightness as usize],
            _ => brightness,
        }
    }

    fn get_scaled_brightness(state: &HwLightState, max_brightness: u32) -> u32 {
        Self::scale_brightness(Self::get_brightness_from_state(state), max_brightness)
    }

    fn read_max_brightness(path: &Path) -> u32 {
        match fs::read_to_string(path) {
            Ok(s) => s.trim().parse::<u32>().unwrap_or_else(|e| {
                warn!("failed to parse max_brightness from {}: {}", path.display(), e);
                0
            }),
            Err(_) => {
                warn!("failed to read from {}", path.display());
                0
            }
        }
    }

    fn write_to_sysfs<P: AsRef<Path>, S: AsRef<str>>(path: P, value: S) -> bool {
        match fs::OpenOptions::new().write(true).open(path.as_ref()) {
            Ok(mut file) => {
                if file.write_all(value.as_ref().as_bytes()).is_err() {
                    warn!("failed to write {} to {}", value.as_ref(), path.as_ref().display());
                    false
                } else {
                    true
                }
            }
            Err(_) => {
                warn!("failed to write {} to {}", value.as_ref(), path.as_ref().display());
                false
            }
        }
    }

    fn handle_backlight(state: &HwLightState) {
        let max_brightness_path = Path::new(LCD_LED_DIR).join(MAX_BRIGHTNESS_FILE);
        let max_brightness = Self::read_max_brightness(&max_brightness_path);

        let brightness = Self::get_scaled_brightness(state, max_brightness);
        debug!("Setting brightness: {}", brightness);

        let brightness_path = Path::new(LCD_LED_DIR).join(BRIGHTNESS_FILE);
        Self::write_to_sysfs(&brightness_path, brightness.to_string());
    }
}

impl ILights for LightsService {
    fn setLightState(&self, id: i32, state: &HwLightState) -> binder::Result<()> {
        match id {
            id if id == LightType::BACKLIGHT.0 as i32 => {
                LightsService::handle_backlight(state);
                Ok(())
            }
            _ => Err(Status::new_exception(ExceptionCode::UNSUPPORTED_OPERATION, None)),
        }
    }

    fn getLights(&self) -> binder::Result<Vec<HwLight>> {
        debug!("getLights called");
        Ok(BACKENDS
            .iter()
            .enumerate()
            .map(|(i, &backend)| HwLight {
                id: backend.0 as i32,
                r#type: backend,
                ordinal: i as i32,
            })
            .collect())
    }
}
