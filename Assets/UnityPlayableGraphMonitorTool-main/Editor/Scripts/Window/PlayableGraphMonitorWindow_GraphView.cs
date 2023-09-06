using GBG.PlayableGraphMonitor.Editor.GraphView;
using UnityEngine.UIElements;

namespace GBG.PlayableGraphMonitor.Editor
{
    public partial class PlayableGraphMonitorWindow
    {
        private PlayableGraphView _graphView;


        private void CreateGraphView(VisualElement container)
        {
            _graphView = new PlayableGraphView
            {
                style =
                {
                    flexGrow = new StyleFloat(1)
                }
            };

            container.Add(_graphView);
        }
    }
}
