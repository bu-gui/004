import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/deepseek_provider.dart';

class AiAssistantPage extends StatefulWidget {
  const AiAssistantPage({super.key});

  @override
  State<AiAssistantPage> createState() => _AiAssistantPageState();
}

class _AiAssistantPageState extends State<AiAssistantPage> {
  final TextEditingController _controller = TextEditingController();
  final ScrollController _scrollController = ScrollController();
  bool _isSending = false;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<DeepSeekProvider>().loadChatHistory();
    });
  }

  @override
  void dispose() {
    _controller.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  void _scrollToBottom() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_scrollController.hasClients) {
        _scrollController.animateTo(
          _scrollController.position.maxScrollExtent,
          duration: const Duration(milliseconds: 300),
          curve: Curves.easeOut,
        );
      }
    });
  }

  Future<void> _sendMessage() async {
    final text = _controller.text.trim();
    if (text.isEmpty || _isSending) return;

    setState(() => _isSending = true);
    _controller.clear();

    final provider = context.read<DeepSeekProvider>();
    await provider.sendChatMessage(text);

    setState(() => _isSending = false);
    _scrollToBottom();
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<DeepSeekProvider>();
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    return Scaffold(
      appBar: AppBar(
        title: const Text('AI 健康助手'),
        centerTitle: true,
      ),
      body: Column(
        children: [
          Expanded(
            child: provider.chatMessages.isEmpty
                ? _buildEmptyState(theme, colorScheme)
                : _buildChatList(provider, theme, colorScheme),
          ),
          _buildInputBar(colorScheme, theme),
        ],
      ),
    );
  }

  Widget _buildEmptyState(ThemeData theme, ColorScheme colorScheme) {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            Icons.smart_toy_outlined,
            size: 80,
            color: colorScheme.primary.withValues(alpha: 0.5),
          ),
          const SizedBox(height: 24),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 32),
            child: Text(
              '您好！我是您的AI健康助手，有什么可以帮助您的？',
              style: theme.textTheme.bodyLarge?.copyWith(
                color: colorScheme.onSurfaceVariant,
                height: 1.5,
              ),
              textAlign: TextAlign.center,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildChatList(
    DeepSeekProvider provider,
    ThemeData theme,
    ColorScheme colorScheme,
  ) {
    _scrollToBottom();

    return ListView.builder(
      controller: _scrollController,
      padding: const EdgeInsets.all(16),
      itemCount: provider.chatMessages.length + (_isSending ? 1 : 0),
      itemBuilder: (context, index) {
        if (index == provider.chatMessages.length && _isSending) {
          return _buildTypingIndicator(colorScheme);
        }

        final message = provider.chatMessages[index];
        final isUser = message.isUser;

        return Padding(
          padding: EdgeInsets.only(
            bottom: index == provider.chatMessages.length - 1 ? 0 : 12,
          ),
          child: Row(
            mainAxisAlignment:
                isUser ? MainAxisAlignment.end : MainAxisAlignment.start,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              if (!isUser) ...[
                CircleAvatar(
                  radius: 16,
                  backgroundColor: colorScheme.primaryContainer,
                  child: Icon(
                    Icons.auto_awesome,
                    size: 18,
                    color: colorScheme.primary,
                  ),
                ),
                const SizedBox(width: 8),
              ],
              Flexible(
                child: Container(
                  constraints: BoxConstraints(
                    maxWidth: MediaQuery.of(context).size.width * 0.75,
                  ),
                  padding: const EdgeInsets.symmetric(
                    horizontal: 16,
                    vertical: 12,
                  ),
                  decoration: BoxDecoration(
                    color: isUser
                        ? colorScheme.primary
                        : colorScheme.surfaceContainerHighest,
                    borderRadius: BorderRadius.only(
                      topLeft: const Radius.circular(20),
                      topRight: const Radius.circular(20),
                      bottomLeft: Radius.circular(
                        isUser ? 20 : 4,
                      ),
                      bottomRight: Radius.circular(
                        isUser ? 4 : 20,
                      ),
                    ),
                  ),
                  child: Text(
                    message.content,
                    style: TextStyle(
                      color: isUser
                          ? colorScheme.onPrimary
                          : colorScheme.onSurface,
                      height: 1.4,
                    ),
                  ),
                ),
              ),
              if (isUser) ...[
                const SizedBox(width: 8),
                CircleAvatar(
                  radius: 16,
                  backgroundColor: colorScheme.primary,
                  child: Icon(
                    Icons.person,
                    size: 18,
                    color: colorScheme.onPrimary,
                  ),
                ),
              ],
            ],
          ),
        );
      },
    );
  }

  Widget _buildTypingIndicator(ColorScheme colorScheme) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 12),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          CircleAvatar(
            radius: 16,
            backgroundColor: colorScheme.primaryContainer,
            child: Icon(
              Icons.auto_awesome,
              size: 18,
              color: colorScheme.primary,
            ),
          ),
          const SizedBox(width: 8),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
            decoration: BoxDecoration(
              color: colorScheme.surfaceContainerHighest,
              borderRadius: const BorderRadius.only(
                topLeft: Radius.circular(20),
                topRight: Radius.circular(20),
                bottomRight: Radius.circular(20),
                bottomLeft: Radius.circular(4),
              ),
            ),
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                _dot(colorScheme, 0),
                const SizedBox(width: 4),
                _dot(colorScheme, 1),
                const SizedBox(width: 4),
                _dot(colorScheme, 2),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _dot(ColorScheme colorScheme, int index) {
    return AnimatedOpacity(
      opacity: 1.0,
      duration: const Duration(milliseconds: 300),
      child: Container(
        width: 8,
        height: 8,
        decoration: BoxDecoration(
          color: colorScheme.onSurfaceVariant,
          shape: BoxShape.circle,
        ),
      ),
    );
  }

  Widget _buildInputBar(ColorScheme colorScheme, ThemeData theme) {
    return Container(
      padding: EdgeInsets.only(
        left: 16,
        right: 8,
        top: 8,
        bottom: MediaQuery.of(context).padding.bottom + 8,
      ),
      decoration: BoxDecoration(
        color: colorScheme.surface,
        border: Border(
          top: BorderSide(
            color: colorScheme.outlineVariant,
            width: 0.5,
          ),
        ),
      ),
      child: Row(
        children: [
          Expanded(
            child: TextField(
              controller: _controller,
              textInputAction: TextInputAction.send,
              onSubmitted: (_) => _sendMessage(),
              decoration: InputDecoration(
                hintText: '输入您的健康问题...',
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(24),
                  borderSide: BorderSide.none,
                ),
                filled: true,
                fillColor: colorScheme.surfaceContainerHighest,
                contentPadding: const EdgeInsets.symmetric(
                  horizontal: 20,
                  vertical: 12,
                ),
              ),
            ),
          ),
          const SizedBox(width: 8),
          IconButton.filled(
            onPressed: _isSending ? null : _sendMessage,
            icon: _isSending
                ? SizedBox(
                    width: 20,
                    height: 20,
                    child: CircularProgressIndicator(
                      strokeWidth: 2,
                      color: colorScheme.onPrimary,
                    ),
                  )
                : const Icon(Icons.send_rounded),
          ),
        ],
      ),
    );
  }
}
