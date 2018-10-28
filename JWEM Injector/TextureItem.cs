using System;
using System.IO;

[Serializable]
public class TextureItem
{
    public string type;
    public int dataIndex;
    public int dataSize;
    public byte[] data;
}