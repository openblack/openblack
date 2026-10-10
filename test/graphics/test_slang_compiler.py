"""Integration checks for the Slang-to-SDL shader compiler, independent of the game."""

import pathlib
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest


COMPILER = pathlib.Path(sys.argv.pop(1)).resolve()
SKY_SOURCE = pathlib.Path(sys.argv.pop(1)).resolve()


class SlangCompilerTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="openblack shader test ")
        self.addCleanup(self.directory.cleanup)
        self.root = pathlib.Path(self.directory.name)
        self.output = self.root / "generated headers"

    def compile(self, *sources, output=None):
        return subprocess.run(
            [str(COMPILER), *map(str, sources), "-o", str(output or self.output)],
            capture_output=True,
            text=True,
            timeout=60,
        )

    def shader(self, name, source):
        path = self.root / (name + ".slang")
        path.write_text(source)
        return path

    def test_sky_layout_and_spirv(self):
        result = self.compile(SKY_SOURCE)
        self.assertEqual(result.returncode, 0, result.stderr)
        header = (self.output / "sky.slang.spv.h").read_text()
        self.assertIn("#pragma once", header)
        self.assertIn(".pitch = 36,", header)
        self.assertIn("SDL_GPU_VERTEXELEMENTFORMAT_SHORT2", header)
        for stage, samplers, uniforms in (("Vertex", 0, 1), ("Fragment", 1, 1)):
            info = re.search(rf"k_Sky{stage}ShaderCreateInfo = .*?\{{(.*?)\}};", header, re.S)[1]
            self.assertIn(f".num_samplers = {samplers},", info)
            self.assertIn(f".num_uniform_buffers = {uniforms},", info)
            data = re.search(rf"k_Sky{stage}Bytes = \{{(.*?)\}};", header, re.S)[1]
            binary = bytes(int(value, 16) for value in re.findall(r"0x([0-9a-f]{2})", data))
            words = struct.unpack(f"<{len(binary) // 4}I", binary)
            self.assertEqual(words[0], 0x07230203)
            if stage == "Vertex":
                # Slang lowers its column-major matrix to a transposed SPIR-V
                # matrix with RowMajor storage and OpVectorTimesMatrix. Together
                # these consume GLM's bytes as mul(mvp, position).
                instructions = []
                offset = 5
                while offset < len(words):
                    size, opcode = words[offset] >> 16, words[offset] & 0xFFFF
                    self.assertGreater(size, 0)
                    instructions.append((opcode, words[offset + 1:offset + size]))
                    offset += size
                self.assertTrue(any(op == 72 and args[2] == 4 for op, args in instructions))
                self.assertTrue(any(op == 144 for op, args in instructions))
            validator = shutil.which("spirv-val")
            if validator:
                path = self.root / f"{stage}.spv"
                path.write_bytes(binary)
                validation = subprocess.run(
                    [validator, "--target-env", "vulkan1.2", str(path)],
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                self.assertEqual(validation.returncode, 0, validation.stderr)

    def test_multiple_sources_and_deterministic_output(self):
        other = self.shader("other", SKY_SOURCE.read_text())
        result = self.compile(SKY_SOURCE, other)
        self.assertEqual(result.returncode, 0, result.stderr)
        before = (self.output / "other.slang.spv.h").read_bytes()
        result = self.compile(other)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(before, (self.output / "other.slang.spv.h").read_bytes())

    def test_missing_source_fails(self):
        result = self.compile(self.root / "missing.slang")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Failed to read shader", result.stderr)

    def test_invalid_shader_fails(self):
        result = self.compile(self.shader("invalid", "not valid shader source!"))
        self.assertNotEqual(result.returncode, 0)

    def test_output_path_is_a_file(self):
        output = self.root / "file"
        output.write_text("existing contents")
        result = self.compile(SKY_SOURCE, output=output)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(output.read_text(), "existing contents")

    def test_header_path_is_a_directory(self):
        (self.output / "sky.slang.spv.h").mkdir(parents=True)
        result = self.compile(SKY_SOURCE)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Failed to open output header", result.stderr)

    def test_compute_stage_is_rejected(self):
        shader = self.shader("compute", '[shader("compute")] [numthreads(1, 1, 1)] void main() {}')
        result = self.compile(shader)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("vertex and one fragment", result.stderr)

    def test_resource_types_are_counted_separately(self):
        shader = self.shader("resources", """
            struct Vertex { float3 position; }
            [[vk::binding(0, 2)]] Sampler2D albedo;
            [[vk::binding(1, 2)]] RWTexture2D<float4> target;
            [[vk::binding(2, 2)]] RWStructuredBuffer<float4> data;
            [shader("vertex")]
            float4 vertexMain(Vertex input) : SV_Position { return float4(input.position, 1); }
            [shader("fragment")]
            float4 fragmentMain(float4 position : SV_Position) : SV_Target
            {
                float4 color = albedo.Sample(position.xy);
                target[uint2(position.xy)] = color;
                data[0] = color;
                return color;
            }
        """)
        result = self.compile(shader)
        self.assertEqual(result.returncode, 0, result.stderr)
        header = (self.output / "resources.slang.spv.h").read_text()
        info = re.search(r"k_ResourcesFragmentShaderCreateInfo = .*?\{(.*?)\};", header, re.S)[1]
        for field in ("samplers", "storage_textures", "storage_buffers"):
            self.assertIn(f".num_{field} = 1,", info)

    def test_gaps_in_bindings_are_rejected(self):
        shader = self.shader("gapped", SKY_SOURCE.read_text().replace("vk::binding(0, 3)", "vk::binding(1, 3)"))
        result = self.compile(shader)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("bindings must be consecutive", result.stderr)


if __name__ == "__main__":
    unittest.main()
