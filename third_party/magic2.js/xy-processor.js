class XYProcessor extends AudioWorkletProcessor {
    constructor() {
        super();

        // 描画ポイント列: [[x, y], [x, y], ...]
        this.points = [];
        this.index = 0;              // 現在の点インデックス（偶数: p0, 奇数: p1）
        this.subPos = 0;             // 線分内サンプル位置
        this.blankSamples = 0;       // 線分間ブランキングサンプル数

        // 距離ベース制御用
        this.segmentLengths = [];    // 各線分の長さ
        this.totalLength = 0;        // 全線分の合計長

        // 設定
        this.minSamplesPerSegment = 2;
        this.speedScale = 16.0; // 1.0 = 等速、0.5 = 2倍速、2.0 = 半速
        this.skipNextProcess = false;

        this.port.onmessage = (event) => {
            if (event.data.type === "setPoints") {
                if (this.skipNextProcess) {
                    return
                }

                this.skipNextProcess = true;
                this.points = event.data.points;
                this.index = 0;
                this.subPos = 0;
                this._setupSegments();
            }
        };
    }

    // 全線分の長さと合計長を事前計算
    _setupSegments() {
        this.segmentLengths = [];
        this.totalLength = 0;

        const N = this.points.length;
        if (N < 2) return;

        for (let i = 0; i < N; i += 2) {
            const p0 = this.points[i];
            const p1 = this.points[(i + 1) % N];

            // ★ 線分クリップ
            const clipped = this.clipSegment(p0, p1);
            if (clipped) {
                const dx = p1[0] - p0[0];
                const dy = p1[1] - p0[1];
                const len = Math.sqrt(dx * dx + dy * dy);

                this.segmentLengths.push(len);
                this.totalLength += len;
            }
        }

        if (this.totalLength <= 0) {
            this.totalLength = 1e-6;
        }
    }

    // 線分と境界の交点を求める
    intersect(p0, p1, bound, isX) {
        const [x0, y0] = p0;
        const [x1, y1] = p1;

        if (isX) {
            const dx = x1 - x0;
            if (dx === 0) return null;
            const t = (bound - x0) / dx;
            if (t < 0 || t > 1) return null;
            return [bound, y0 + (y1 - y0) * t];
        } else {
            const dy = y1 - y0;
            if (dy === 0) return null;
            const t = (bound - y0) / dy;
            if (t < 0 || t > 1) return null;
            return [x0 + (x1 - x0) * t, bound];
        }
    }

    clipSegment(p0, p1) {
        const inside = (p) =>
            p[0] >= -1 && p[0] <= 1 && p[1] >= -1 && p[1] <= 1;

        // まず両端が内側ならそのまま返す
        const p0Inside = inside(p0);
        const p1Inside = inside(p1);

        // 交点を集める
        const bounds = [
            [-1, true],  // x = -1
            [1, true],  // x =  1
            [-1, false], // y = -1
            [1, false], // y =  1
        ];

        let intersections = [];

        for (const [bound, isX] of bounds) {
            const p = this.intersect(p0, p1, bound, isX);
            if (p) intersections.push(p);
        }

        // ★ ケース1：両端が内側 → そのまま描画
        if (p0Inside && p1Inside) {
            return [p0, p1];
        }

        // ★ ケース2：両端が外側
        if (!p0Inside && !p1Inside) {
            if (intersections.length === 2) {
                // p0→p1 の順序で並べる
                const [i0, i1] = intersections.sort((a, b) => {
                    const ta = (a[0] - p0[0]) ** 2 + (a[1] - p0[1]) ** 2;
                    const tb = (b[0] - p0[0]) ** 2 + (b[1] - p0[1]) ** 2;
                    return ta - tb;
                });
                return [i0, i1];
            }
            return null; // 完全に外側
        }

        // ★ ケース3：p0 外側 → p1 内側
        if (!p0Inside && p1Inside) {
            if (intersections.length > 0) {
                // p0 に最も近い交点を使う
                const i0 = intersections.sort((a, b) => {
                    const ta = (a[0] - p0[0]) ** 2 + (a[1] - p0[1]) ** 2;
                    const tb = (b[0] - p0[0]) ** 2 + (b[1] - p0[1]) ** 2;
                    return ta - tb;
                })[0];
                return [i0, p1];
            }
            return null;
        }

        // ★ ケース4：p0 内側 → p1 外側
        if (p0Inside && !p1Inside) {
            if (intersections.length > 0) {
                // p1 に最も近い交点を使う
                const i0 = intersections.sort((a, b) => {
                    const ta = (a[0] - p1[0]) ** 2 + (a[1] - p1[1]) ** 2;
                    const tb = (b[0] - p1[0]) ** 2 + (b[1] - p1[1]) ** 2;
                    return ta - tb;
                })[0];
                return [p0, i0];
            }
            return null;
        }

        return null;
    }


    process(inputs, outputs) {
        const outL = outputs[0][0];
        const outR = outputs[0][1];

        const N = this.points.length;
        if (N < 2 || this.totalLength <= 0) {
            outL.fill(0);
            outR.fill(0);
            this.skipNextProcess = false;
            return true;
        }

        const frameSamples = outL.length;

        // 全長をこのフレームのサンプル数に収めるスケール
        const scale = (frameSamples / this.totalLength) * this.speedScale;

        for (let i = 0; i < frameSamples; i++) {

            const p0 = this.points[this.index];
            const p1 = this.points[(this.index + 1) % N];

            const clipped = this.clipSegment(p0, p1);

            // ★ クリップで完全に消えた
            if (!clipped) {
                this.subPos = 0;

                if(this.index + 2 >= N)
                    this.skipNextProcess = false;
                this.index = (this.index + 2) % N;
                outL[i] = 0;
                outR[i] = 0;
                continue;
            }

            const [a, b] = clipped;

            const dx = b[0] - a[0];
            const dy = b[1] - a[1];
            const len = Math.sqrt(dx * dx + dy * dy);

            // ★ クリップ後の長さが 0 → 次へ
            if (len <= 1e-9) {
                this.subPos = 0;
                this.index = (this.index + 2) % N;
                outL[i] = 0;
                outR[i] = 0;
                continue;
            }

            // ★ クリップ後の長さでサンプル数を決める
            const samplesPerSegment = Math.max(
                this.minSamplesPerSegment,
                Math.floor(len * scale)
            );

            const t = this.subPos / samplesPerSegment;

            const x = a[0] + dx * t;
            const y = a[1] + dy * t;

            outL[i] = x;
            outR[i] = y;

            this.subPos++;

            // ★ 線分終了 → 次へ
            if (this.subPos >= samplesPerSegment + this.blankSamples) {
                this.subPos = 0;
                if(this.index + 2 >= N)
                    this.skipNextProcess = false;
                this.index = (this.index + 2) % N;
            }
        }

        return true;
    }

    process2(inputs, outputs) {
        const outL = outputs[0][0];
        const outR = outputs[0][1];

        const N = this.points.length;
        if (N < 2) {
            outL.fill(0);
            outR.fill(0);
            return true;
        }

        // ★ 1 unit を描くのに必要なサンプル数（描画速度）
        //    例: 1 unit を 1 秒で描く → sampleRate
        const speedSamplesPerUnit = sampleRate * this.speedUnitsPerSecond;

        for (let i = 0; i < outL.length; i++) {

            const p0 = this.points[this.index];
            const p1 = this.points[(this.index + 1) % N];

            // ★ 線分クリップ
            const clipped = this.clipSegment(p0, p1);

            let x = 0, y = 0;

            if (clipped) {
                const [a, b] = clipped;

                // ★ 線分の長さ
                const dx = b[0] - a[0];
                const dy = b[1] - a[1];
                const len = Math.sqrt(dx * dx + dy * dy);

                // ★ この線分に必要なサンプル数（距離ベース）
                this.samplesPerSegment = Math.max(
                    2,
                    Math.floor(len * speedSamplesPerUnit)
                );

                // ★ 線分上の位置 t
                const t = this.subPos / this.samplesPerSegment;

                x = a[0] + dx * t;
                y = a[1] + dy * t;
            }

            outL[i] = x;
            outR[i] = y;

            this.subPos++;

            // ★ 線分終了 → 次の線分へ
            if (this.subPos >= this.samplesPerSegment + this.blankSamples) {
                this.subPos = 0;
                this.index = (this.index + 2) % N;
            }
        }

        return true;
    }

}

registerProcessor("xy-processor", XYProcessor);
