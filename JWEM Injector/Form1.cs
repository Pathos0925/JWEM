using DirectXTexNet;
using GijSoft.DllInjection;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using SharpDX.Direct3D;
using SharpDX.Direct3D11;

//using Google.Protobuf;

using Device = SharpDX.Direct3D11.Device;
using System.Xml.Serialization;

namespace JWEM_Injector
{
    public partial class Form1 : Form
    {
        public string[] TextureTypes = new string[]
        {
            "Albedo",
            "NormalMap",
            "Packed",
            //"Albedo L",
            //"1",
            //"2",
            //"3"
        };
        public string[] SpeciesList = new string[]
        {
            "Allosaurus",
            "Ankylosaurus",
            "Archaeornithomimus",
            "Apatosaurus",
            "Archaeornithomimus",
            "Baryonyx",
            "Brachiosaurus",
            "Camarasaurus",
            "Carnotaurus",
            "Ceratosaurus",
            "Chasmosaurus",
            "Chungkingosaurus",
            "Corythosaurus",
            "Crichtonsaurus",
            "Deinonychus",
            "Dilophosaurus",
            "Diplodocus",
            "Dracorex",
            "Edmontosaurus",
            "Gallimimus",
            "Giganotosaurus",
            "Gigantspinosaurus",
            "Huayangosaurus",
            "Indominus Rex",
            "Indoraptor",
            "Kentrosaurus",
            "Maiasaura",
            "Majungasaurus",
            "Mamenchisaurus",
            "Metriacanthosaurus",
            "Muttaburrasaurus",
            "Nodosaurus",
            "Pachycephalosaurus",
            "Parasaurolophus",
            "Pentaceratops",
            "Polacanthus",
            "Sauropelta",
            "Sinoceratops",
            "Stegosaurus",
            "Spinosaurus",
            "Struthiomimus",
            "Stygimoloch",
            "Styracosaurus",
            "Suchomimus",
            "Torosaurus",
            "Triceratops",
            "Tsintaosaurus",
            "Tyrannosaurus",
            "Velociraptor",
        };
        public string[] ResolutionList = new string[]
        {
            "1024",
            "2048"
        };

        public List<TextureItem> textureItems = new List<TextureItem>();


        DllInjector injector;

        public static string ProcessName = "JWE";

        string currentDirectory;

        bool waitingForClose = true;
        bool injected = false;
        int waitCount = 0;

        public Form1()
        {
            InitializeComponent();
        }

        private void button2_Click(object sender, EventArgs e)
        {

        }

        private void Form1_Load(object sender, EventArgs e)
        {           
            findApplicationTimer.Enabled = true;
            textureTypeDropdown.Items.AddRange(TextureTypes);
            textureTypeDropdown.SelectedIndex = 0;
            textureTypeDropdown.Update();

            speciesDropdown.Items.AddRange(SpeciesList);
            speciesDropdown.SelectedIndex = 0;
            speciesDropdown.Update();

            resDropdown.Items.AddRange(ResolutionList);
            resDropdown.SelectedIndex = 0;
            resDropdown.Update();

            currentDirectory = System.IO.Path.GetDirectoryName(Application.ExecutablePath);
            Console.WriteLine(currentDirectory);

        }

        private bool IsProcessRunning()
        {
            System.Diagnostics.Process[] proc = System.Diagnostics.Process.GetProcessesByName(ProcessName);
            if (proc.Length > 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        private void findApplicationTimer_Tick(object sender, EventArgs e)
        {
            if (!enabledCheckBox.Checked)
                return;

            if (!injected)
            {
                if (waitingForClose)
                {
                    if (IsProcessRunning())
                    {
                        statusLabel.Text = "Please close the game first!";
                        return;
                    }
                    else
                    {
                        waitingForClose = false;
                    }
                }

                waitCount++;
                statusLabel.Text = "Waiting for " + ProcessName + " to start...";

                if (IsProcessRunning())
                {
                    statusLabel.Text = "Injecting...";
                    var result = DllInjector.GetInstance.Inject(ProcessName, currentDirectory + "\\JWEM.dll");
                    if (result != DllInjectionResult.Success)
                    {
                        statusLabel.Text = "Injection Error: " + result;
                    }
                    else
                    {
                        statusLabel.Text = "Injection Successful!";
                    }
                    injected = true;
                }
            }
            else
            {

            }
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
            var skinName = speciesDropdown.SelectedItem.ToString() + " Skin" + ".JWEMS";
            saveFileDialog1.Filter = "JWE Mod Skin(*.JWEMS)|*.JWEMS";
            saveFileDialog1.FileName = skinName;
            if (saveFileDialog1.ShowDialog() == DialogResult.OK)
            {
                //buildSkinTextures.Species = speciesDropdown.SelectedItem.ToString();


                MemoryStream output = new MemoryStream();
                output.Write(Encoding.UTF8.GetBytes(speciesDropdown.SelectedItem.ToString() + "\n"), 0, speciesDropdown.SelectedItem.ToString().Length + 1);
                output.Write(Encoding.UTF8.GetBytes(textureItems.Count.ToString() + "\n"), 0, textureItems.Count.ToString().Length + 1);

                for (int i = 0; i < textureItems.Count; i++)
                {
                    var typeBytes = Encoding.UTF8.GetBytes(textureItems[i].type + "\n");
                    output.Write(typeBytes, 0, typeBytes.Length);
                    output.Write(Encoding.UTF8.GetBytes(textureItems[i].data.Length.ToString() + "\n"), 0, textureItems[i].data.Length.ToString().Length + 1);

                    output.Write(textureItems[i].data, 0, textureItems[i].data.Length);
                }

                output.Position = 0;

                

                using (var file = File.Create(saveFileDialog1.FileName))
                {
                    //SevenZip.Compression.LZMA.Encoder coder = new SevenZip.Compression.LZMA.Encoder();
                    //coder.WriteCoderProperties(file);
                    //file.Write(BitConverter.GetBytes(output.Length), 0, 8);
                    //coder.Code(output, file, output.Length, -1, null);
                    output.CopyTo(file);
                }

                output.Flush();
                output.Close();


            }
        }

       
        private void btnAddTexture_Click(object sender, EventArgs e)
        {

            if (textureTypeDropdown.SelectedIndex < 0)
            {
                //MessageBox.Show("Please select a texture type");
                buildStatusLbl.Text = "Please select a texture type!";
                return;
            }
            string textureType = textureTypeDropdown.SelectedItem.ToString();



            openFileDialog1.Filter = "Image Files(*.PNG;*.DDS)|*.PNG;*.DDS|All files (*.*)|*.*";
            openFileDialog1.Title = "Add a texture";
            if (openFileDialog1.ShowDialog() == DialogResult.OK)
            {
                if (string.IsNullOrEmpty(openFileDialog1.FileNames[0]))
                {
                    return;
                }

                DisableButtons(true);
                ScratchImage image;
                if (Path.GetExtension(openFileDialog1.FileNames[0]).ToUpper() == ".DDS")
                {
                    image = TexHelper.Instance.LoadFromDDSFile(openFileDialog1.FileNames[0], DDS_FLAGS.NONE);
                }
                else
                {
                    image = TexHelper.Instance.LoadFromWICFile(openFileDialog1.FileNames[0], WIC_FLAGS.NONE);
                }

                ScratchImage finalImage = image;
                var metadata = image.GetMetadata();

                if (TexHelper.Instance.IsCompressed(metadata.Format))
                {
                    Console.WriteLine("Decompressing...");
                    buildStatusLbl.Text = "Decompressing...";
                    finalImage = finalImage.Decompress(DXGI_FORMAT.UNKNOWN);
                }

                if (finalImage.GetImageCount() < 11)
                {
                    Console.WriteLine("GenerateMipMaps...");
                    buildStatusLbl.Text = "GenerateMipMaps...";
                    finalImage = finalImage.GenerateMipMaps(TEX_FILTER_FLAGS.DEFAULT, 11);
                }

                if (useGPUChk.Checked)
                {
                    Console.WriteLine("Compressing with GPU...");
                    buildStatusLbl.Text = "Compressing with GPU...";
                    var device = new Device(DriverType.Hardware, DeviceCreationFlags.None);
                    var context = device.ImmediateContext;
                    finalImage = finalImage.Compress(device.NativePointer, GetRequiredFormat(textureType),
                        TEX_COMPRESS_FLAGS.PARALLEL,
                        0.5f);
                    Console.WriteLine("Complete.");
                }
                else
                {
                    Console.WriteLine("Compressing with CPU, this could take a while");
                    buildStatusLbl.Text = "Compressing with CPU, this could take a while...";
                    finalImage = finalImage.Compress(GetRequiredFormat(textureType),
                        TEX_COMPRESS_FLAGS.PARALLEL,
                        0.5f);
                    Console.WriteLine("Complete.");
                }

                metadata = finalImage.GetMetadata();

                TextureItem newItem = new TextureItem();
                var stream = finalImage.SaveToDDSMemory(DDS_FLAGS.NONE);
                newItem.data = ReadFully(stream);
                newItem.type = textureType;
                textureItems.Add(newItem);
                UpdateList();

                buildStatusLbl.Text = "Press Save to save file.";


                DisableButtons(false);

            }
        }

        void DisableButtons(bool disable)
        {
            btnAddTexture.Enabled = !disable;
            btnRemoveTexture.Enabled = !disable;
            btnSave.Enabled = !disable;
            
        }

        void UpdateList()
        {
            listBox1.Items.Clear();
            for (int i = 0; i < textureItems.Count; i++)
            {
                listBox1.Items.Add(textureItems[i].type);
            }
        }

        DXGI_FORMAT GetRequiredFormat(string textureType)
        {
            switch(textureType)
            {
                case "Albedo":
                    return DXGI_FORMAT.BC7_UNORM_SRGB;
                case "NormalMap":
                    return DXGI_FORMAT.BC5_UNORM;
                case "Packed":
                    return DXGI_FORMAT.BC7_UNORM;
                case "Albedo L":
                    return DXGI_FORMAT.BC1_TYPELESS;
                default:
                    return DXGI_FORMAT.BC7_UNORM_SRGB;
            }
        }

        private void btnRemoveTexture_Click(object sender, EventArgs e)
        {
            if (listBox1.SelectedIndex < 0)
                return;

            if (listBox1.SelectedIndex <= textureItems.Count)
            {
                textureItems.RemoveAt(listBox1.SelectedIndex);
            }
            UpdateList();
        }

        public static byte[] ReadFully(Stream input)
        {
            byte[] buffer = new byte[16 * 1024];
            using (MemoryStream ms = new MemoryStream())
            {
                int read;
                while ((read = input.Read(buffer, 0, buffer.Length)) > 0)
                {
                    ms.Write(buffer, 0, read);
                }
                return ms.ToArray();
            }
        }

        private void useGPUChk_CheckedChanged(object sender, EventArgs e)
        {
            if (!useGPUChk.Checked)
            {
                buildStatusLbl.Text = "Compressing on the CPU is slow, and may take longer";
            }
        }
    }
}