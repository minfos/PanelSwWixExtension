using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_BackupAndRestore : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_BackupAndRestore), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_BackupAndRestore));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column)
                    , new ColumnDefinition(nameof(Component_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1)
                    , new ColumnDefinition(nameof(Path), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Flags), ColumnType.Number, 2, false, false, ColumnCategory.Integer, 0, 127)
                };
            }
        }

        public PSW_BackupAndRestore() : base(SymbolDefinition)
        { }

        public PSW_BackupAndRestore(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "bnr")
        { }

        public string Component_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string Path
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public ushort Flags
        {
            get => (ushort)Fields[2].AsNumber();
            set => Fields[2].Set(value);
        }
    }
}
