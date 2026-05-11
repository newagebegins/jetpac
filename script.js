let gl;
let ext;
let lastTimeMs;
let frameTimeElement = document.getElementById("frame-time");

function main()
{
    const memory = new WebAssembly.Memory({ initial: 30 });

    Promise.all([
        WebAssembly.instantiateStreaming(fetch("game.wasm"), { env: { memory } }),
        fetch('jetpac.atls').then(response => response.arrayBuffer()),
    ]).then(([wasmModule, atlasBuffer]) => {
        const [
            atlasWidth,
            atlasHeight,
            bitmapInfosSize,
            atlasPixelsSize,
            atlasInfosOffset,
            atlasPixelsOffset,
        ] = Array.from(new Uint32Array(atlasBuffer, 0, 6).values());
        
        const pixels = new Uint8Array(atlasBuffer, atlasPixelsOffset, atlasPixelsSize);

        let memoryView = new DataView(memory.buffer);
        let memoryUint8 = new Uint8Array(memory.buffer);

        // Memory map:
        // struct game_memory (20)
        // struct game_input (44)
        // permanentStorage (permanentStorageSize)
        // bitmap_info (bitmapInfosSize)
        // renderList (renderListSize)

        let gameMemoryOffset = 0;
        let gameMemorySize = 20;

        let gameInputOffset = gameMemoryOffset + gameMemorySize;
        let gameInputSize = 44;

        let permanentStorageOffset = gameInputOffset + gameInputSize;
        let permanentStorageSize = 4096;

        let bitmapInfosOffset = permanentStorageOffset + permanentStorageSize;

        let renderListOffset = bitmapInfosOffset + bitmapInfosSize;
        let renderListSize = 8192;

        const gameMemory = new Uint32Array(memory.buffer, gameMemoryOffset, gameMemorySize);
        gameMemory.set([
            permanentStorageOffset,
            permanentStorageSize,
            renderListOffset,
            renderListSize,
            0, // RenderListUsed
        ]);

        const gameInput = new Uint32Array(memory.buffer, gameInputOffset, gameInputSize);

        let infosSource = new Uint8Array(atlasBuffer, atlasInfosOffset, bitmapInfosSize);
        memoryUint8.set(infosSource, bitmapInfosOffset);

        let screenWidth = 256;
        let screenHeight = 192;
        let screenScale = 2;

        let canvas = document.getElementById("game-canvas");
        canvas.width = screenWidth*screenScale;
        canvas.height = screenHeight*screenScale;

        gl = canvas.getContext("webgl");
        if(!gl)
        {
            alert("ERROR: WebGL is not supported in your browser");
            return;
        }

        ext = gl.getExtension('ANGLE_instanced_arrays');
        if(!ext)
        {
            alert('ANGLE_instanced_arrays is not supported');
            return;
        }

        const vertexShaderSource = `
            attribute vec2 position;
            attribute vec2 scale;   // instanced
            attribute vec2 offset;  // instanced
            attribute vec2 uvScale;   // instanced
            attribute vec2 uvOffset;   // instanced
            uniform mat4 screenToClip;
            varying vec2 uv;

            void main()
            {
                uv = uvScale*position.xy + uvOffset;
                gl_Position = screenToClip*vec4(scale*position + offset, 0.0, 1.0);
            }
        `;

        const fragmentShaderSource = `
            precision highp float;
            varying vec2 uv;
            uniform sampler2D image;

            void main()
            {
                gl_FragColor = texture2D(image, uv);
            }
        `;

        let vertexShader = gl.createShader(gl.VERTEX_SHADER);
        let fragmentShader = gl.createShader(gl.FRAGMENT_SHADER);

        gl.shaderSource(vertexShader, vertexShaderSource);
        gl.shaderSource(fragmentShader, fragmentShaderSource);

        gl.compileShader(vertexShader);
        gl.compileShader(fragmentShader);

        let program = gl.createProgram();
        gl.attachShader(program, vertexShader);
        gl.attachShader(program, fragmentShader);
        gl.linkProgram(program);

        if(gl.getProgramParameter(program, gl.LINK_STATUS))
        {
            gl.deleteShader(vertexShader);
            gl.deleteShader(fragmentShader);

            let vertexBuffer = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
            let quadVertices = new Float32Array([
                1, 1,
                0, 1,
                1, 0,
                0, 0,
            ]);
            gl.bufferData(gl.ARRAY_BUFFER, quadVertices, gl.STATIC_DRAW);
            let positionLocation = gl.getAttribLocation(program, "position");
            gl.enableVertexAttribArray(positionLocation);
            gl.vertexAttribPointer(positionLocation, 2, gl.FLOAT, false, 0, 0);

            let instanceBuffer = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, instanceBuffer);
            let instances = new Float32Array([
                // instance 0
                32, 32, // scale
                100, 50, // offset
                1, 1, // uv scale
                0, 0, // uv offset

                // instance 1
                64, 64, // scale
                0, 0, // offset
                0.5, 0.5, // uv scale
                0, 0.5, // uv offset
                //-0.5, 0.5, // uv scale
                //0.5, 0.5, // uv offset
            ]);
            gl.bufferData(gl.ARRAY_BUFFER, instances, gl.DYNAMIC_DRAW);

            let stride = 8*4;

            let scaleLocation = gl.getAttribLocation(program, "scale");
            gl.enableVertexAttribArray(scaleLocation);
            gl.vertexAttribPointer(scaleLocation, 2, gl.FLOAT, false, stride, 0);
            ext.vertexAttribDivisorANGLE(scaleLocation, 1);

            let offsetLocation = gl.getAttribLocation(program, "offset");
            gl.enableVertexAttribArray(offsetLocation);
            gl.vertexAttribPointer(offsetLocation, 2, gl.FLOAT, false, stride, 2*4);
            ext.vertexAttribDivisorANGLE(offsetLocation, 1);

            let uvScaleLocation = gl.getAttribLocation(program, "uvScale");
            gl.enableVertexAttribArray(uvScaleLocation);
            gl.vertexAttribPointer(uvScaleLocation, 2, gl.FLOAT, false, stride, 4*4);
            ext.vertexAttribDivisorANGLE(uvScaleLocation, 1);

            let uvOffsetLocation = gl.getAttribLocation(program, "uvOffset");
            gl.enableVertexAttribArray(uvOffsetLocation);
            gl.vertexAttribPointer(uvOffsetLocation, 2, gl.FLOAT, false, stride, 6*4);
            ext.vertexAttribDivisorANGLE(uvOffsetLocation, 1);

            let screenToClipLocation = gl.getUniformLocation(program, "screenToClip");
            console.assert(screenToClipLocation !== null);

            gl.useProgram(program);

            let screenToClip = [
                2/screenWidth,0,0,0,
                0,2/screenHeight,0,0,
                0,0,1,0,
                -1,-1,0,1,
            ];
            gl.uniformMatrix4fv(screenToClipLocation, false, screenToClip);

            let texture = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, texture);
            gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, atlasWidth, atlasHeight, 0, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);

            gl.enable(gl.BLEND);
            gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

            lastTimeMs = performance.now();
            mainLoop(lastTimeMs);

            function mainLoop(currentTimeMs)
            {
                requestAnimationFrame(mainLoop);

                let deltaMs = currentTimeMs - lastTimeMs;
                lastTimeMs = currentTimeMs;

                frameTimeElement.textContent = deltaMs.toFixed(2);

                let maxDeltaMs = 17;
                if(deltaMs > maxDeltaMs)
                {
                    deltaMs = maxDeltaMs;
                }

                let dt = deltaMs / 1000;

                wasmModule.instance.exports.GameUpdateAndRender(gameMemoryOffset, gameInputOffset, bitmapInfosOffset);

                gl.clearColor(0.0, 0.0, 0.3, 1.0);
                gl.clear(gl.COLOR_BUFFER_BIT);

                let vertsPerInstance = 4;
                let instanceCount = 2;

                ext.drawArraysInstancedANGLE(gl.TRIANGLE_STRIP, 0, vertsPerInstance, instanceCount);
            }
        }
        else
        {
            console.error("Link failed: ", gl.getProgramInfoLog(program));
            console.error("Vertex shader log: ", gl.getShaderInfoLog(vertexShader));
            console.error("Fragment shader log: ", gl.getShaderInfoLog(fragmentShader));
        }
    });
}

main();
