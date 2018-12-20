﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text;
using System.Xml;
using Microsoft.Tools.WindowsInstallerXml;

namespace PanelSw.Wix.Extensions
{
    class PanelSwPreProcessor : PreprocessorExtension
    {
        private string[] prefixes_ = new string[] { "tuple", "endtuple", "tuple_range", "heat" };
        public override string[] Prefixes => prefixes_;

        Dictionary<string, List<string>> tuples_ = new Dictionary<string, List<string>>();
        public override void FinalizePreprocess()
        {
            tuples_.Clear();
        }

        public override void InitializePreprocess()
        {
            tuples_.Clear();
        }

        public override bool ProcessPragma(SourceLineNumberCollection sourceLineNumbers, string prefix, string pragma, string args, XmlWriter writer)
        {
            switch (prefix)
            {
                case "tuple":
                    if (tuples_.ContainsKey(pragma))
                    {
                        throw new WixException(WixErrors.PreprocessorExtensionPragmaFailed(sourceLineNumbers, pragma, "Pragma is nested within same pragma"));
                    }

                    List<string> values = new List<string>(args.Split(';'));
                    if (values.Count == 0)
                    {
                        throw new WixException(WixErrors.PreprocessorExtensionPragmaFailed(sourceLineNumbers, pragma, "No values specified. Expected '<?pragma tuple.KEY val1;val2;...;valX?>'"));
                    }

                    tuples_[pragma] = values;
                    return true;

                case "endtuple":
                    if (!tuples_.Remove(pragma))
                    {
                        throw new WixException(WixErrors.PreprocessorExtensionPragmaFailed(sourceLineNumbers, pragma, $"endtuple pragma for undefined tuple"));
                    }
                    return true;

                case "heat":
                    ProcessHeat(sourceLineNumbers, pragma, args, writer);
                    return true;

                default:
                    return false;
            }
        }

        private void ProcessHeat(SourceLineNumberCollection sourceLineNumbers, string pragma, string args, XmlWriter writer)
        {
            string xsltPath = null;
            string outPath = null;
            try
            {
                Assembly caller = Assembly.GetAssembly(typeof(PreprocessorExtension));
                string heatPath = caller.Location;
                heatPath = Path.GetDirectoryName(heatPath);
                heatPath = Path.Combine(heatPath, "heat.exe");
                if (!File.Exists(heatPath))
                {
                    Core.OnMessage(WixErrors.FileNotFound(sourceLineNumbers, heatPath));
                    return;
                }

                // Expand preprocessor variables
                args = Core.PreprocessString(sourceLineNumbers, args);
                Core.OnMessage(new WixGenericMessageEventArgs(sourceLineNumbers, 0, MessageLevel.Information, $"Executing heat command: \"{heatPath}\" {args}"));

                // Load xslt from resource
                xsltPath = Path.GetTempFileName();
                Assembly me = Assembly.GetExecutingAssembly();
                using (Stream xslt = me.GetManifestResourceStream("PanelSw.Wix.Extensions.Data.WixToInclude.xslt"))
                {
                    using (FileStream fs = File.OpenWrite(xsltPath))
                    {
                        byte[] data = new byte[xslt.Length];
                        xslt.Read(data, 0, data.Length);
                        fs.Write(data, 0, data.Length);
                    }
                }

                // Set temp output file and apply transform
                outPath = Path.GetTempFileName();
                args += $" -t \"{xsltPath}\" -o \"{outPath}\"";

                ProcessStartInfo heatArgs = new ProcessStartInfo(heatPath, $"{pragma} {args}");
                heatArgs.UseShellExecute = false;
                heatArgs.RedirectStandardError = true;
                heatArgs.RedirectStandardOutput = true;
                Process heat = Process.Start(heatArgs);
                heat.WaitForExit();
                if (heat.ExitCode != 0)
                {
                    string err = heat.StandardOutput.ReadToEnd();
                    err += heat.StandardError.ReadToEnd();
                    Core.OnMessage(WixErrors.PreprocessorError(sourceLineNumbers, err));
                    return;
                }

                // Include heat file
                XmlDocument doc = new XmlDocument();
                doc.Load(outPath);
                foreach (XmlNode e in doc.DocumentElement.ChildNodes)
                {
                    writer.WriteProcessingInstruction(Preprocessor.LineNumberElementName, sourceLineNumbers.EncodedSourceLineNumbers);
                    writer.WriteRaw(e.OuterXml);
                }
            }
            catch (Exception ex)
            {
                Core.OnMessage(WixErrors.PreprocessorError(sourceLineNumbers, ex.Message));
            }
            finally
            {
                if (!string.IsNullOrEmpty(outPath) && File.Exists(outPath))
                {
                    File.Delete(outPath);
                }
                if (!string.IsNullOrEmpty(xsltPath) && File.Exists(xsltPath))
                {
                    File.Delete(xsltPath);
                }
            }
        }

        public override string EvaluateFunction(string prefix, string key, string[] args)
        {
            int i;
            switch (prefix)
            {
                case "tuple":
                    if (!tuples_.ContainsKey(key))
                    {
                        return null;
                    }
                    if (args.Length != 1)
                    {
                        return null;
                    }
                    if (!int.TryParse(args[0], out i) || (i < 0) || (i >= tuples_[key].Count))
                    {
                        return null;
                    }
                    return tuples_[key][i];

                case "tuple_range":
                    if (!tuples_.ContainsKey(key))
                    {
                        return null;
                    }
                    string range = "";
                    for (i = 0; i < tuples_[key].Count; ++i)
                    {
                        if (i > 0)
                        {
                            range += ";";
                        }
                        range += i.ToString();
                    }
                    return range;

                default:
                    return null;

            }
        }

        public override string GetVariableValue(string prefix, string name)
        {
            switch (prefix)
            {
                case "tuple":
                    int i = name.LastIndexOf('.');
                    if (i <= 1)
                    {
                        return null;
                    }
                    string key = name.Substring(0, i);
                    string arg = name.Substring(i + 1);
                    return EvaluateFunction(prefix, key, new string[] { arg });

                case "tuple_range":
                    return EvaluateFunction(prefix, name, null);

                default:
                    return null;

            }
        }
    }
}