using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_XmlSearch : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_XmlSearch), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_XmlSearch));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Property_), ColumnType.String, 0, false, false, ColumnCategory.Text, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(FilePath), ColumnType.String, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Expression), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Language), ColumnType.String, 0, false, true, ColumnCategory.Text, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Namespaces), ColumnType.String, 0, false, true, ColumnCategory.Text, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Match), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_XmlSearch() : base(SymbolDefinition)
        { }

        public PSW_XmlSearch(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "xml")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public string FilePath
        {
            get => Fields[1].AsString();
            set => Fields[1].Set(value);
        }

        public string Expression
        {
            get => Fields[2].AsString();
            set => Fields[2].Set(value);
        }

        public string Language
        {
            get => Fields[3].AsString();
            set => Fields[3].Set(value);
        }

        public string Namespaces
        {
            get => Fields[4].AsString();
            set => Fields[4].Set(value);
        }

        public int Match
        {
            get => Fields[5].AsNumber();
            set => Fields[5].Set(value);
        }

        public string Condition
        {
            get => Fields[6].AsString();
            set => Fields[6].Set(value);
        }
    }
}
