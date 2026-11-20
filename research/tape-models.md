# Tape stage reference research — Saturator 0.2

## Studer A80 (15 and 30 ips)
Studer A80/RC manufacturer literature specifies CCIR/NAB equalisation, 150 kHz bias/erase oscillators, and frequency-response tolerances depending on speed: 30 ips 50 Hz–20 kHz ±2 dB; 15 ips 50 Hz–18 kHz ±2 dB; 7.5 ips 50 Hz–15 kHz ±2 dB. 1 kHz distortion figures depend on reference flux and equalisation, so avoid extrapolating one figure to all levels. Source: https://tile.loc.gov/storage-services/master/mbrs/recording_preservation/manuals/Studer%20A80-RC%20Professional%20Tape%20Recorders.pdf

Independent contemporary review describes unusually small head-contour effects and very clean electronics, supporting subtle low-frequency bump and mild nonlinear saturation at nominal operating levels. Source: https://www.worldradiohistory.com/Archive-All-Audio/Archive-Studio-Sound/70s/Studio-Sound-1976-01.pdf

## Nakamichi Dragon (high-quality cassette reference)
Manufacturer service manual: 1-7/8 ips, metal ZX tape response 20 Hz–22 kHz ±3 dB at -20 dB recording level, wow/flutter <0.019% weighted RMS, THD <0.8% at 400 Hz/0 dB ZX, 105 kHz bias. These are *specified test conditions*, not unconditional response. Source: https://www.richardhess.com/manuals/Nakamichi/nak_Dragon_Service_Manual_hi_res.pdf

Dragon model should be wideband and clean when lightly driven; HF compression and modulation grow at higher levels. Dolby B/C is not implemented in Saturator 0.2.

## Vintage valve deck
The user's tentative 'R37' reference has not been unambiguously identified. This model is currently an explicitly generic vintage valve recorder, not a verified R37, and uses stronger low-frequency head contour, HF rolloff and saturation.

## Current implementation limits
Saturator 0.2 includes behavioural stage voicings, nonlinear magnetic memory and level-dependent frequency effects. No physical core constants have been fitted; transformer flux law is an approximation, tape record/reproduce EQ is not a complete electrical simulation, wow/flutter and hiss are not implemented. No hardware measurements or listening validation. Do not market as exact emulations.
