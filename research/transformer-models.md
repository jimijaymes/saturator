# Transformer modelling research — Neve/Carnhill, UREI 1176, API

Status: literature-derived research and proposed modelling targets, **not hardware-calibrated emulations**. Source confidence and electrical conditions must be preserved. The Saturator 0.1 transformer DSP is a generic behavioural approximation and does NOT yet implement these component models.

## API historical microphone input: AP2622
- Historical API 312-series microphone input transformer. Secondary technical sources identify 1:7 and 1:3.5 voltage-ratio configurations (winding strapping).
- Typical historical documentation associates 150/600-ohm input configurations with ~10k secondary load; these are *nominal matching impedances*, not winding DC resistances.
- Loading network matters: the primary decoupling capacitor, secondary termination and Zobel network affect HF resonance, damping, and effective frequency response.
- A CineMag document describing the Reichenbach RE-0887 modification reports replacing the AP2622 (1:7) with a 1:8 winding, omitting the usual primary 1000pF capacitors and the secondary Zobel in favour of a resistive load. This is a **modified preamp** and must not be treated as a measurement of stock API response.
- Need primary inductance, leakage inductance, winding resistance, capacitances, frequency-response plots at multiple levels and THD-vs-frequency for calibration.

## API 2503 output
- Quadfilar four-winding transformer; configurable 1:1, 1:2, 1:3 voltage-ratio options.
- API 312 review describes ~9.5 dB difference between 1:1 and 1:3, enabling amplifier drive with lower external output level.
- Model together with API 2520 discrete op-amp and its output impedance/current limits; a transformer-only block cannot reproduce the complete API preamp.
- Investigate primary current and source impedance, loading, core material and winding coupling before fitting any harmonic profile.

## API AP2516 input and API 2623 output
- AP2516 is documented as the input transformer on API's current 312; avoid assuming it is identical to vintage AP2622.
- API parts list includes AP2516-E input and AP2503, 2623-1-4 and 2623-1-6 output replacement parts.
- Secondary reference gives 2623 1:1 and 1:2 configurations. Magnetic distortion data not established.

## Neve / Carnhill
- VTB9045 (Neve 10468 equivalent): turns 1+1:2+2, primary DCR 24+24 ohms, secondary DCR 130+130 ohms, configurable microphone impedance/winding arrangements.
- VTB1148 (LO1166 equivalent): turns 1+1:1.7+1.7, primary DCR 6+6 ohms, secondary DCR 20+20 ohms; high-level gapped output transformer. Carnhill guide lists source/load arrangements and voltage gains.
- Actual wiring matters: an RN73 build guide identifies 180 pF across VTB9045 pins 7/10, 2.2 nF for line input VTB9046 and a 1.5k + 0.01uF Zobel on VTB1148. These are *one documented build implementation*, not universal values.
- Louis Woods, 'The Measurement of Audio Transformer Colouration' (University of Huddersfield) reports tests of VTB9045 and VTB1148. Use the original plots and exact measurement conditions to constrain nonlinear parameters; do not infer numerical THD curves from prose.

## UREI 1176
- Early units used Peerless 600-ohm input transformers, later UTC O-12; Rev E schematic explicitly marks UTC O-12 as 2:1 equivalent.
- Early Class A output transformer UA-5002 / 5002A includes tertiary feedback and emitter windings, so feedback loop and amplifier interaction are integral to its character.
- Do not conflate Rev D output with later Rev F Class AB output and transformer.

## Proposed engineering approach
1. Implement separate *electrical* configurations: turns ratio, series winding resistances, primary magnetising inductance, leakage inductance, parasitic capacitance, source/load impedances, and Zobel.
2. Add a **stateful nonlinear flux** branch: dPhi/dt proportional to induced voltage; use a bounded, physically stable magnetisation law with optional hysteresis and bias. Do not equate a static tanh waveshaper with magnetic hysteresis.
3. Drive models at a documented voltage reference: dBu and dBFS mapping must be explicit. Low-frequency saturation is strongly dependent on applied V/f.
4. Validate small-signal frequency/phase response, 20/50/100/1000 Hz THD versus level, 2-tone IMD, load dependence, oversampling aliasing and CPU cost.
5. Classify each parameter as (A) manufacturer specification, (B) schematic or academic measurement, (C) secondary-source claim, (D) fitted estimate. Never label D as measured.
6. Keep input and output transformer models separate; API 2503 winding selection must change the output network and gain, not merely the nonlinear drive.

## References
- Carnhill design guide: https://www.barryrudolph.com/recall/manuals/carnhill_design_guide.pdf
- Transformer colouration thesis: https://pure.hud.ac.uk/files/71763779/FINAL_THESIS_Louis_Woods.pdf
- API 312 review: https://www.soundonsound.com/reviews/api-312
- API 312 product specifications: https://apiaudio.com/product/312-mic-preamp/
- API parts list: https://service.apiaudio.com/parts
- API 512c manual: https://apiaudio.com/docs/manuals/512c_user_2019-05-26.pdf
- CineMag / RE-0887 account: https://cinemag.biz/news/CAPI/rack-and-roll-ad.pdf
- Transformer ratio cross-reference (secondary): https://www.diyrecordingequipment.com/blogs/news/tagged/basic-info
- 1176 revision history: https://www.mixonline.com/recording/1176-revision-history-372048
- 1176 Rev E schematic: https://thehistoryofrecording.com/Manuals/UREI/Urei%201176LN%2001.pdf
- Carnhill wiring example: https://www.rackneve.com/node/890
